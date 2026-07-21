#!/usr/bin/env python3
"""Carte de prévisualisation des biomes — Aethelgard. 10-step pipeline."""

import math, random, argparse, sys
from dataclasses import dataclass
from typing import Tuple
import noise
from PIL import Image

PARAMS = {
    "macro_scale": 0.0004, "macro_amplitude": 160.0, "macro_octaves": 2,
    "macro_persistence": 0.5, "macro_lacunarity": 4.0,
    "base_shape_scale": 0.001, "base_shape_amplitude": 15.0,
    "base_shape_persistence": 0.5, "base_shape_lacunarity": 2.0,
    "mountain_folly_scale": 0.002, "mountain_folly_amplitude": 350.0, "mountain_folly_bias": 0.63,
    "biome_sep_scale": 0.04, "biome_sep_octaves": 2,
    "temp_scale": 0.0003, "humid_scale": 0.001, "glacier_threshold": 0.25,
    "temp_perturb_scale": 0.005, "temp_perturb_amplitude": 0.15,
    "humid_perturb_scale": 0.005, "humid_perturb_amplitude": 0.15,
    # Ice age: 0.0=None, 1.0=full (shifts effective glacier threshold toward 1)
    "ice_age_factor": 0.1,
    # Affinity-based scoring: distance-to-ideal-point with Gaussian falloff
    "temp_weight": 1.0, "humid_weight": 1.0, "height_weight": 1.0, "affinity_sharpness": 3.0,
    # Forest: prefers cool+humid
    "forest_temp_affinity": 0.75, "forest_humid_affinity": 0.8, "forest_height_affinity": 0.5, "forest_adjust": 1,
    # Desert: prefers hot+dry
    "desert_temp_affinity": 0.9, "desert_humid_affinity": 0.15, "desert_height_affinity": 0.5, "desert_adjust": 1.2,
    # Plains: generalist
    "plains_temp_affinity": 0.5, "plains_humid_affinity": 0.5, "plains_height_affinity": 0.5, "plains_adjust": 0.6,
    "meso_scale": 0.02, "meso_amplitude": 8.0, "meso_octaves": 2,
    "meso_persistence": 0.5, "meso_lacunarity": 2.0,
    "micro_scale": 0.08, "micro_amplitude": 1.0, "micro_octaves": 1,
    "micro_persistence": 0.5, "micro_lacunarity": 2.0,
    "global_elevation": 85.0,
    "sea_level": 80.0, "mountain_start": 120.0, "max_height": 220.0,
    "plains_hill_scale": 0.03, "plains_hill_amplitude": 5.0,
    "desert_dune_scale": 0.015, "desert_dune_amplitude": 6.0,
    "mountain_detail_scale": 0.01, "mountain_detail_amplitude": 12.0,
    "mountain_lift_scale": 0.004, "mountain_lift_amplitude": 40.0,
    "mountain_rough_scale": 0.015, "mountain_rough_threshold": 0.35,
    "mountain_rough_amplitude": 15.0, "mountain_rough_detail_scale": 0.008,
    "mountain_rock_threshold": 130.0, "mountain_snow_threshold": 170.0,
    "biome_blend_distance": 25.0,
    # Lakes: threshold [0,1] controls chance, diameter controls circle size in blocks
    "lake_noise_threshold": 0.65, "lake_circle_diameter": 44.0, "lake_depth": 6, "lake_depth_slope": 0.05,
    # Rivers: threshold [0,1] controls chance, diameter controls circle size in blocks
    "river_noise_threshold": 0.08, "river_circle_diameter": 38.0, "river_depth": 3.0,
    "sea_depth_slope": 0.05, "sea_max_depth": 25.0,
    "sea_floor_scale": 0.02, "sea_floor_amplitude": 5.0,
    "beach_width": 15.0,
}

N_MACRO, N_BASESHAPE, N_MOUNTAIN_FOLLY = 0, 1, 2
N_BIOME_SEP, N_TEMPERATURE, N_HUMIDITY = 3, 4, 5
N_TEMP_PERTURB, N_HUMID_PERTURB = 6, 7
N_MESO, N_MICRO = 8, 9
N_PLAINS_HILL, N_DESERT_DUNE, N_MOUNTAIN_DETAIL = 10, 11, 12
N_SEA_FLOOR, N_MOUNTAIN_ROUGH = 13, 14
N_LAKE, N_RIVER = 15, 16
N_PERTURB1, N_PERTURB2 = 17, 18

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
        lake_scale = 1.0/max(p["lake_circle_diameter"], 1.0)
        river_scale = 1.0/max(p["river_circle_diameter"], 1.0)
        ln = perlin(float(wx), float(wy), lake_scale, 1, seed+N_LAKE*7919)
        rn = abs(perlin(float(wx), float(wy), river_scale, 1, seed+N_RIVER*7919))
        if abs(ln) > p["lake_noise_threshold"]:
            height = max(1.0, min(height-p["lake_depth"], p["max_height"]))
            ws = int(min(max(height_pre_relief, 0), 255))
        elif rn < p["river_noise_threshold"] and height >= p["sea_level"]+2:
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
