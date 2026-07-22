#!/usr/bin/env python3
"""Carte de prévisualisation des biomes — Aethelgard. 10-step pipeline."""

import math, random, argparse, sys
from dataclasses import dataclass
from typing import Tuple
import noise
from PIL import Image

PARAMS = {
    "macro_scale": 0.0001, "macro_amplitude": 220.0, "macro_octaves": 2,
    "macro_persistence": 0.5, "macro_lacunarity": 4.0,
    "base_shape_scale": 0.001, "base_shape_amplitude": 15.0,
    "base_shape_persistence": 0.5, "base_shape_lacunarity": 2.0,
    "mountain_folly_scale": 0.002, "mountain_folly_amplitude": 350.0, "mountain_folly_bias": 0.63,
    "biome_sep_scale": 0.04, "biome_sep_octaves": 2,
    "temp_scale": 0.0001, "humid_scale": 0.0005, "glacier_threshold": 0.25,
    "temp_perturb_scale": 0.001, "temp_perturb_amplitude": 0.15,
    "humid_perturb_scale": 0.001, "humid_perturb_amplitude": 0.15,
    "ice_age_factor": 0.1,
    "temp_weight": 1.0, "humid_weight": 1.0, "height_weight": 1.0, "affinity_sharpness": 3.0,
    "forest_temp_affinity": 0.75, "forest_humid_affinity": 0.8, "forest_height_affinity": 0.5, "forest_adjust": 1,
    "desert_temp_affinity": 0.9, "desert_humid_affinity": 0.15, "desert_height_affinity": 0.45, "desert_adjust": 1.2,
    "plains_temp_affinity": 0.5, "plains_humid_affinity": 0.5, "plains_height_affinity": 0.5, "plains_adjust": 0.6,
    "meso_scale": 0.02, "meso_amplitude": 8.0, "meso_octaves": 2,
    "meso_persistence": 0.5, "meso_lacunarity": 2.0,
    "micro_scale": 0.08, "micro_amplitude": 1.0, "micro_octaves": 1,
    "micro_persistence": 0.5, "micro_lacunarity": 2.0,
    "global_elevation": 85.0,
    "sea_level": 80.0, "mountain_start": 150.0, "max_height": 220.0,
    "plains_hill_scale": 0.03, "plains_hill_amplitude": 5.0,
    "desert_dune_scale": 0.015, "desert_dune_amplitude": 6.0,
    "mountain_detail_scale": 0.01, "mountain_detail_amplitude": 12.0,
    "mountain_lift_scale": 0.004, "mountain_lift_amplitude": 40.0,
    "mountain_rough_scale": 0.015, "mountain_rough_threshold": 0.35,
    "mountain_rough_amplitude": 15.0, "mountain_rough_detail_scale": 0.008,
    "mountain_rock_threshold": 152.0, "mountain_snow_threshold": 170.0,
    "biome_blend_distance": 25.0,
    # Lakes: Voronoi-based system
    "voronoi_scale": 0.001, "lake_probability": 0.5,
    "lake_max_depth": 10.0, "lake_depth_falloff": 2.0,
    "lake_min_diameter": 100.0, "lake_max_diameter": 5000.0,
    # Rivers: legacy Perlin noise (scale x20, depth x5)
    "river_noise_threshold": 0.05, "river_circle_diameter": 760.0, "river_depth": 10.0,
    "sea_depth_slope": 0.05, "sea_max_depth": 25.0,
    "sea_floor_scale": 0.02, "sea_floor_amplitude": 5.0,
    "beach_width": 15.0,
    # Shore deformation
    "shore_deform_scale": 0.05, "shore_deform_amplitude": 3.0,
}

N_MACRO, N_BASESHAPE, N_MOUNTAIN_FOLLY = 0, 1, 2
N_BIOME_SEP, N_TEMPERATURE, N_HUMIDITY = 3, 4, 5
N_TEMP_PERTURB, N_HUMID_PERTURB = 6, 7
N_MESO, N_MICRO = 8, 9
N_PLAINS_HILL, N_DESERT_DUNE, N_MOUNTAIN_DETAIL = 10, 11, 12
N_SEA_FLOOR, N_MOUNTAIN_ROUGH = 13, 14
N_LAKE, N_RIVER = 15, 16
N_VORONOI, N_SHORE_DEFORM = 17, 18
N_PERTURB1, N_PERTURB2 = 19, 20

_offset_cache = {}
def _make_offset(seed): rng = random.Random(seed); return (rng.random()*10000, rng.random()*10000)

def perlin(x, y, scale, octaves, seed, persistence=0.5, lacunarity=2.0):
    v, amp, ma, f = 0.0, 1.0, 0.0, 1.0
    for i in range(octaves):
        key = seed + i*7919
        if key not in _offset_cache: _offset_cache[key] = _make_offset(key)
        ox, oy = _offset_cache[key]
        v += amp * noise.pnoise2(x*scale*f+ox, y*scale*f+oy, octaves=1, repeatx=1024, repeaty=1024, base=0)
        ma += amp; amp *= persistence; f *= lacunarity
    return v / ma

def voronoi(x, y, scale, seed):
    sx = x * scale
    sy = y * scale
    cell_x = int(math.floor(sx))
    cell_y = int(math.floor(sy))
    min_d1 = float('inf')
    min_d2 = float('inf')
    closest_id = 0
    for dy in range(-1, 2):
        for dx in range(-1, 2):
            cx = cell_x + dx
            cy = cell_y + dy
            hash_key = (cx * 374761393 + cy * 668265263) ^ (seed * 1274126177)
            rng = random.Random(hash_key)
            px = cx + rng.random()
            py = cy + rng.random()
            d = math.sqrt((sx - px)**2 + (sy - py)**2)
            if d < min_d1:
                min_d2 = min_d1
                min_d1 = d
                closest_id = hash_key
            elif d < min_d2:
                min_d2 = d
    return closest_id, min_d1, min_d2, min_d2 - min_d1

def base_height(wx, wy, p, seed):
    m = perlin(float(wx), float(wy), p["macro_scale"], p["macro_octaves"], seed+N_MACRO*7919, p["macro_persistence"], p["macro_lacunarity"])
    bs = abs(perlin(float(wx), float(wy), p["base_shape_scale"], 1, seed+N_BASESHAPE*7919, p["base_shape_persistence"], p["base_shape_lacunarity"]))
    return p["global_elevation"] + m*p["macro_amplitude"] + bs*p["base_shape_amplitude"]

def score_biome(t, h, world_height, p, temp_aff, humid_aff, height_aff, adjust):
    nh = max(0.0, min((world_height-1.0)/(p["max_height"]-1.0), 1.0))
    dist = p["temp_weight"]*abs(t-temp_aff) + p["humid_weight"]*abs(h-humid_aff) + p["height_weight"]*abs(nh-height_aff)
    return math.exp(-dist*dist*p["affinity_sharpness"])*adjust

def select_biome(wx, wy, p, seed):
    height = max(1.0, min(base_height(wx, wy, p, seed), p["max_height"]))
    if height < p["sea_level"]: return 0, 0.0
    if height >= p["mountain_start"]: return 2, max(0.0, min((height-p["mountain_start"])/p["biome_blend_distance"], 1.0))
    temp = max(0.0, min((perlin(float(wx), float(wy), p["temp_scale"], 1, seed+N_TEMPERATURE*7919)+1)*0.5, 1.0))
    humid = max(0.0, min((perlin(float(wx), float(wy), p["humid_scale"], 1, seed+N_HUMIDITY*7919)+1)*0.5, 1.0))
    temp = max(0.0, min(temp + perlin(float(wx), float(wy), p["temp_perturb_scale"], 1, seed+N_TEMP_PERTURB*7919)*p["temp_perturb_amplitude"], 1.0))
    humid = max(0.0, min(humid + perlin(float(wx), float(wy), p["humid_perturb_scale"], 1, seed+N_HUMID_PERTURB*7919)*p["humid_perturb_amplitude"], 1.0))
    eff_glacier = p["glacier_threshold"] + p["ice_age_factor"]*(1.0-p["glacier_threshold"])
    if temp < eff_glacier: biome = 4
    else:
        fs = score_biome(temp, humid, height, p, p["forest_temp_affinity"], p["forest_humid_affinity"], p["forest_height_affinity"], p["forest_adjust"])
        ds = score_biome(temp, humid, height, p, p["desert_temp_affinity"], p["desert_humid_affinity"], p["desert_height_affinity"], p["desert_adjust"])
        ps = score_biome(temp, humid, height, p, p["plains_temp_affinity"], p["plains_humid_affinity"], p["plains_height_affinity"], p["plains_adjust"])
        if fs>=ds and fs>=ps: biome=3
        elif ds>=ps: biome=1
        else: biome=0

    ds_h=height-p["sea_level"]; dm_h=p["mountain_start"]-height; dg_h=abs(temp-eff_glacier)
    g = max(0.0, min(min(ds_h,dm_h,dg_h)/p["biome_blend_distance"], 1.0))
    return biome, g

@dataclass
class ColumnResult: height: float; biome: int; water_surface: int

def folly_contribution(wx, wy, scale, amplitude, bias, t, seed):
    raw = perlin(float(wx), float(wy), scale, 1, seed)
    n = (raw+1.0)*0.5
    m = max(n-bias, 0.0)
    return m*amplitude*t

def compute_column(wx, wy, p, seed):
    height = max(1.0, min(base_height(wx, wy, p, seed), p["max_height"]))
    if height >= p["sea_level"]:
        t = max(0.0, min((height-p["sea_level"])/20.0, 1.0))
        fs = seed+N_MOUNTAIN_FOLLY*7919
        c1 = folly_contribution(wx, wy, p["mountain_folly_scale"], p["mountain_folly_amplitude"], p["mountain_folly_bias"], t, fs)
        c2 = folly_contribution(wx, wy, p["mountain_folly_scale"]*0.8, p["mountain_folly_amplitude"]*0.8, p["mountain_folly_bias"], t, fs+1)
        c3 = folly_contribution(wx, wy, p["mountain_folly_scale"]*1.1, p["mountain_folly_amplitude"]*1.1, p["mountain_folly_bias"], t, fs+2)
        height += max(c1, c2, c3)
        height = max(1.0, min(height, p["max_height"]))
    is_sea = height < p["sea_level"]
    is_mountain = height >= p["mountain_start"]
    height_pre_relief = height
    biome = 0
    ws = 0
    if is_sea:
        sf = perlin(float(wx), float(wy), p["sea_floor_scale"], 1, seed+N_SEA_FLOOR*7919)*p["sea_floor_amplitude"]
        raw = p["sea_level"]-height
        d = raw if raw<=p["sea_max_depth"] else p["sea_max_depth"]+(raw-p["sea_max_depth"])*p["sea_depth_slope"]
        height = p["sea_level"]-d+sf
        height = max(1.0, min(height, p["max_height"]))
        ws = int(min(max(p["sea_level"],0),255))
        biome = 0
    if is_mountain:
        biome = 2
        mg = max(0.0, min((height-p["mountain_start"])/p["biome_blend_distance"], 1.0))
        height += perlin(float(wx), float(wy), p["mountain_detail_scale"], 2, seed+N_MOUNTAIN_DETAIL*7919)*p["mountain_detail_amplitude"]*mg
        mf = max(0.0, min((height-p["mountain_start"])/(p["max_height"]-p["mountain_start"]), 1.0))
        lift = math.sin(mf * math.pi * 0.5)
        height += perlin(float(wx), float(wy), p["mountain_lift_scale"], 1, seed+N_MOUNTAIN_DETAIL*7919+31337)*p["mountain_lift_amplitude"]*lift
        rn = perlin(float(wx), float(wy), p["mountain_rough_scale"], 1, seed+N_MOUNTAIN_ROUGH*7919)
        if rn > p["mountain_rough_threshold"]:
            rough_mask = (rn-p["mountain_rough_threshold"])/(1.0-p["mountain_rough_threshold"])
            d1 = perlin(float(wx), float(wy), p["mountain_rough_detail_scale"], 1, seed+N_MOUNTAIN_ROUGH*7919+7901)
            d2 = perlin(float(wx), float(wy), p["mountain_rough_detail_scale"]*2.0, 1, seed+N_MOUNTAIN_ROUGH*7919+7907)
            height += (d1*0.6+d2*0.4)*p["mountain_rough_amplitude"]*rough_mask*mf
        height = max(1.0, min(height, p["max_height"]))
    if not is_sea and not is_mountain:
        temp = max(0.0, min((perlin(float(wx), float(wy), p["temp_scale"], 1, seed+N_TEMPERATURE*7919)+1)*0.5, 1.0))
        humid = max(0.0, min((perlin(float(wx), float(wy), p["humid_scale"], 1, seed+N_HUMIDITY*7919)+1)*0.5, 1.0))
        temp = max(0.0, min(temp + perlin(float(wx), float(wy), p["temp_perturb_scale"], 1, seed+N_TEMP_PERTURB*7919)*p["temp_perturb_amplitude"], 1.0))
        humid = max(0.0, min(humid + perlin(float(wx), float(wy), p["humid_perturb_scale"], 1, seed+N_HUMID_PERTURB*7919)*p["humid_perturb_amplitude"], 1.0))
        eff_glacier = p["glacier_threshold"] + p["ice_age_factor"]*(1.0-p["glacier_threshold"])
        if temp < eff_glacier: biome = 4
        else:
            fs = score_biome(temp, humid, height, p, p["forest_temp_affinity"], p["forest_humid_affinity"], p["forest_height_affinity"], p["forest_adjust"])
            ds = score_biome(temp, humid, height, p, p["desert_temp_affinity"], p["desert_humid_affinity"], p["desert_height_affinity"], p["desert_adjust"])
            ps = score_biome(temp, humid, height, p, p["plains_temp_affinity"], p["plains_humid_affinity"], p["plains_height_affinity"], p["plains_adjust"])
            if fs>=ds and fs>=ps: biome=3
            elif ds>=ps: biome=1
            else: biome=0
        ds_h=height-p["sea_level"]; dm_h=p["mountain_start"]-height; dg_h=abs(temp-eff_glacier)
        g = max(0.0, min(min(ds_h,dm_h,dg_h)/p["biome_blend_distance"], 1.0))
        if biome == 0: height += abs(perlin(float(wx), float(wy), p["plains_hill_scale"], 1, seed+N_PLAINS_HILL*7919))*p["plains_hill_amplitude"]*g
        elif biome == 1: height += abs(perlin(float(wx), float(wy), p["desert_dune_scale"], 1, seed+N_DESERT_DUNE*7919))*p["desert_dune_amplitude"]*g
        height_pre_relief = height
    height += perlin(float(wx), float(wy), p["meso_scale"], p["meso_octaves"], seed+N_MESO*7919, p["meso_persistence"], p["meso_lacunarity"])*p["meso_amplitude"]
    height += perlin(float(wx), float(wy), p["micro_scale"], p["micro_octaves"], seed+N_MICRO*7919, p["micro_persistence"], p["micro_lacunarity"])*p["micro_amplitude"]
    if height >= p["sea_level"]:
        cell_id, f1, f2, edge = voronoi(float(wx), float(wy), p["voronoi_scale"], seed+N_VORONOI*7919)
        cell_radius = 1.0 / max(p["voronoi_scale"], 0.001)
        if f1 <= cell_radius * 0.6:
            hash_val = (cell_id * 374761393) ^ (seed * 1274126177)
            rng = random.Random(hash_val)
            if rng.random() < p["lake_probability"]:
                sample_radius = cell_radius * 0.9
                min_boundary_h = float('inf')
                for i in range(16):
                    angle = i / 16.0 * 2.0 * math.pi
                    sx = wx + math.cos(angle) * sample_radius
                    sy = wy + math.sin(angle) * sample_radius
                    h = perlin(sx, sy, 0.0004, 2, seed+N_MACRO*7919)
                    h = p["global_elevation"] + h * p["macro_amplitude"]
                    bs = abs(perlin(sx, sy, 0.001, 1, seed+N_BASESHAPE*7919))
                    h += bs * p["base_shape_amplitude"]
                    h += perlin(sx, sy, p["meso_scale"], p["meso_octaves"], seed+N_MESO*7919, p["meso_persistence"], p["meso_lacunarity"]) * p["meso_amplitude"]
                    h += perlin(sx, sy, p["micro_scale"], p["micro_octaves"], seed+N_MICRO*7919, p["micro_persistence"], p["micro_lacunarity"]) * p["micro_amplitude"]
                    min_boundary_h = min(min_boundary_h, h)
                center_h = perlin(float(wx), float(wy), 0.0004, 2, seed+N_MACRO*7919)
                center_h = p["global_elevation"] + center_h * p["macro_amplitude"]
                center_bs = abs(perlin(float(wx), float(wy), 0.001, 1, seed+N_BASESHAPE*7919))
                center_h += center_bs * p["base_shape_amplitude"]
                center_h += perlin(float(wx), float(wy), p["meso_scale"], p["meso_octaves"], seed+N_MESO*7919, p["meso_persistence"], p["meso_lacunarity"]) * p["meso_amplitude"]
                center_h += perlin(float(wx), float(wy), p["micro_scale"], p["micro_octaves"], seed+N_MICRO*7919, p["micro_persistence"], p["micro_lacunarity"]) * p["micro_amplitude"]
                max_water = min(min_boundary_h, center_h + p["lake_max_depth"])
                if height < max_water:
                    norm_dist = max(0.0, min(f1 / (cell_radius * 0.5), 1.0))
                    depth_factor = (1.0 - norm_dist) ** p["lake_depth_falloff"]
                    shore_noise = perlin(float(wx), float(wy), p["shore_deform_scale"], 2, seed+N_SHORE_DEFORM*7919) * p["shore_deform_amplitude"]
                    effective_spill = min_boundary_h + shore_noise
                    if height < effective_spill:
                        water_level = min(effective_spill, center_h + p["lake_max_depth"] * depth_factor)
                        if water_level > height:
                            diameter = f1 * 2.0 / p["voronoi_scale"]
                            if diameter >= p["lake_min_diameter"] and diameter <= p["lake_max_diameter"]:
                                ws = int(min(max(water_level, 0), 255))
        if ws == 0:
            river_scale = 1.0/max(p["river_circle_diameter"], 1.0)
            rn = abs(perlin(float(wx), float(wy), river_scale, 1, seed+N_RIVER*7919))
            if rn < p["river_noise_threshold"] and height >= p["sea_level"]+2:
                rf = 1.0-(rn/p["river_noise_threshold"])
                rp = height
                height = max(1.0, min(height-rf*p["river_depth"], p["max_height"]))
                ws = int(min(max(rp-1, 0), 255))
    height = max(1.0, min(height, p["max_height"]))
    return ColumnResult(height=height, biome=biome, water_surface=ws)

def biome_color(biome, height, water, p):
    if water > 0:
        if biome == 4: return (180, 210, 240)
        return (30, 100, 200)
    if height < p["sea_level"]: return (30, 80, 160)
    if biome == 0: return (34, 139, 34)
    if biome == 1: return (210, 180, 100)
    if biome == 2:
        if height > p["mountain_snow_threshold"]: return (240, 240, 240)
        if height > p["mountain_rock_threshold"]: return (160, 160, 160)
        return (80, 130, 60)
    if biome == 3: return (20, 100, 20)
    if biome == 4: return (200, 220, 240)
    return (255, 0, 255)

def generate_map(size, p, seed):
    img = Image.new("RGB", (size, size)); pix = img.load()
    wr, step = 32768, 32768/size
    print(f"{size}x{size} seed={seed} ...")
    for py in range(size):
        if py%128==0: print(f"  line {py}/{size}")
        wy = int((py-size/2)*step)
        for px in range(size):
            wx = int((px-size/2)*step)
            col = compute_column(wx, wy, p, seed)
            pix[px, py] = biome_color(col.biome, col.height, col.water_surface, p)
    return img

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--size", type=int, default=1024); ap.add_argument("--seed", type=int, default=0); ap.add_argument("--out", type=str, default="carte.png")
    a = ap.parse_args()
    img = generate_map(a.size, PARAMS, a.seed); img.save(a.out)
    print(f"\nSaved: {a.out}")

if __name__ == "__main__": main()
