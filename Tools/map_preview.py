#!/usr/bin/env python3
"""Carte de prévisualisation des biomes — Aethelgard. Nouveau système."""

import math, random, argparse, os
from dataclasses import dataclass, field
from collections import deque
import noise
from PIL import Image, ImageDraw, ImageFont

PARAMS = {
    "macro_scale": 0.0005, "macro_amplitude": 90.0, "macro_octaves": 3,
    "macro_persistence": 0.8, "macro_lacunarity": 3.0,
    "base_shape_scale": 0.001, "base_shape_amplitude": 90.0,
    "base_shape_persistence": 0.5, "base_shape_lacunarity": 2.0,
    "mountain_folly_scale": 0.002, "mountain_folly_amplitude": 280.0, "mountain_folly_bias": 0.55,
    "temp_scale": 0.0001, "humid_scale": 0.001,
    "temp_perturb_scale": 0.001, "temp_perturb_amplitude": 0.15,
    "humid_perturb_scale": 0.001, "humid_perturb_amplitude": 0.15,
    "glacier_threshold": 0.30, "ice_age_factor": 0.1,
    "temp_weight": 1.0, "humid_weight": 1.0, "height_weight": 1.0, "affinity_sharpness": 3.0,
    "forest_temp_affinity": 0.75, "forest_humid_affinity": 0.8, "forest_height_affinity": 0.4, "forest_adjust": 1.0,
    "desert_temp_affinity": 0.9, "desert_humid_affinity": 0.15, "desert_height_affinity": 0.35, "desert_adjust": 1.0,
    "plains_temp_affinity": 0.5, "plains_humid_affinity": 0.5, "plains_height_affinity": 0.35, "plains_adjust": 0.8,
    "ice_mtn_temp_affinity": 0.2, "ice_mtn_humid_affinity": 0.5, "ice_mtn_height_affinity": 0.65, "ice_mtn_adjust": 1.1,
    "humid_mtn_temp_affinity": 0.5, "humid_mtn_humid_affinity": 0.9, "humid_mtn_height_affinity": 0.7, "humid_mtn_adjust": 1.0,
    "classic_mtn_temp_affinity": 0.5, "classic_mtn_humid_affinity": 0.5, "classic_mtn_height_affinity": 0.65, "classic_mtn_adjust": 1.0,
    "meso_scale": 0.01, "meso_amplitude": 12.0, "meso_octaves": 2,
    "meso_persistence": 0.5, "meso_lacunarity": 2.0,
    "micro_scale": 0.08, "micro_amplitude": 1.0, "micro_octaves": 1,
    "micro_persistence": 0.5, "micro_lacunarity": 2.0,
    "global_elevation": 85.0,
    "sea_level": 85.0, "mountain_start": 150.0, "max_height": 260.0,
    "plains_hill_scale": 0.03, "plains_hill_amplitude": 8.0,
    "desert_dune_scale": 0.015, "desert_dune_amplitude": 10.0,
    "mountain_detail_scale": 0.01, "mountain_detail_amplitude": 12.0,
    "mountain_lift_scale": 0.004, "mountain_lift_amplitude": 40.0,
    "mountain_rough_scale": 0.015, "mountain_rough_threshold": 0.35,
    "mountain_rough_amplitude": 20.0, "mountain_rough_detail_scale": 0.008,
    "mountain_rock_threshold": 152.0, "mountain_snow_threshold": 170.0,
    "ice_mtn_peak_amplitude": 55.0,
    "classic_mtn_lift_amplitude": 30.0,
    "humid_mtn_hill_amplitude": 10.0, "humid_mtn_hill_scale": 0.008,
    "biome_blend_distance": 25.0,
    "sea_depth_slope": 0.03, "sea_max_depth": 40.0,
    "sea_floor_scale": 0.02, "sea_floor_amplitude": 5.0,
    "perturb_scale": 0.01,
}

N_MACRO, N_BASESHAPE, N_MOUNTAIN_FOLLY = 0, 1, 2
N_TEMPERATURE, N_HUMIDITY = 4, 5
N_TEMP_PERTURB, N_HUMID_PERTURB = 6, 7
N_MESO, N_MICRO = 8, 9
N_PLAINS_HILL, N_DESERT_DUNE, N_MOUNTAIN_DETAIL = 10, 11, 12
N_SEA_FLOOR, N_MOUNTAIN_ROUGH = 13, 14
N_PERTURB1, N_PERTURB2 = 19, 20

_offset_cache = {}
def _make_offset(seed):
    rng = random.Random(seed)
    return (rng.random() * 10000, rng.random() * 10000)

def perlin(x, y, scale, octaves, seed, persistence=0.5, lacunarity=2.0):
    v, amp, ma, f = 0.0, 1.0, 0.0, 1.0
    for i in range(octaves):
        key = seed + i * 7919
        if key not in _offset_cache:
            _offset_cache[key] = _make_offset(key)
        ox, oy = _offset_cache[key]
        v += amp * noise.pnoise2(x*scale*f+ox, y*scale*f+oy, octaves=1, repeatx=1024, repeaty=1024, base=0)
        ma += amp; amp *= persistence; f *= lacunarity
    return v / ma

def base_height(wx, wy, p, seed):
    m = perlin(float(wx), float(wy), p["macro_scale"], p["macro_octaves"],
               seed + N_MACRO * 7919, p["macro_persistence"], p["macro_lacunarity"])
    bs = abs(perlin(float(wx), float(wy), p["base_shape_scale"], 1,
                    seed + N_BASESHAPE * 7919, p["base_shape_persistence"], p["base_shape_lacunarity"]))
    noise = m * p["macro_amplitude"] + bs * p["base_shape_amplitude"]
    raw = p["global_elevation"] + noise
    d = raw - p["sea_level"]

    coast_amp = 3.0
    sea_att = 3.0
    land_att = 10.0
    blend_sea = 10.0
    blend_land = 7.0

    if d < -blend_sea:
        factor = 1.0 / sea_att
    elif d < 0.0:
        factor = (1.0 / sea_att) + (coast_amp - 1.0 / sea_att) * (d + blend_sea) / blend_sea
    elif d <= blend_land:
        factor = coast_amp + (1.0 / land_att - coast_amp) * d / blend_land
    else:
        factor = 1.0 / land_att

    return p["global_elevation"] + noise * factor

def score_biome(t, h, world_height, p, temp_aff, humid_aff, height_aff, adjust):
    nh = max(0.0, min((world_height - 1.0) / (p["max_height"] - 1.0), 1.0))
    dist = (p["temp_weight"] * abs(t - temp_aff)
          + p["humid_weight"] * abs(h - humid_aff)
          + p["height_weight"] * abs(nh - height_aff))
    return math.exp(-dist * dist * p["affinity_sharpness"]) * adjust

BIOME_COLDPLACE, BIOME_PLAINS, BIOME_DESERT, BIOME_FOREST = 3, 0, 1, 2
BIOME_ICE_MTN, BIOME_HUMID_MTN, BIOME_CLASSIC_MTN = 4, 5, 6

def select_biome(wx, wy, p, seed):
    height = max(1.0, min(base_height(wx, wy, p, seed), p["max_height"]))
    if height < p["sea_level"]:
        return BIOME_PLAINS, 0.0

    temp = max(0.0, min((perlin(float(wx), float(wy), p["temp_scale"], 1,
        seed + N_TEMPERATURE * 7919) + 1) * 0.5, 1.0))
    humid = max(0.0, min((perlin(float(wx), float(wy), p["humid_scale"], 1,
        seed + N_HUMIDITY * 7919) + 1) * 0.5, 1.0))
    temp = max(0.0, min(temp + perlin(float(wx), float(wy), p["temp_perturb_scale"], 1,
        seed + N_TEMP_PERTURB * 7919) * p["temp_perturb_amplitude"], 1.0))
    humid = max(0.0, min(humid + perlin(float(wx), float(wy), p["humid_perturb_scale"], 1,
        seed + N_HUMID_PERTURB * 7919) * p["humid_perturb_amplitude"], 1.0))

    eff_glacier = p["glacier_threshold"] + p["ice_age_factor"] * (1.0 - p["glacier_threshold"])
    if temp < eff_glacier:
        biome = BIOME_COLDPLACE
    else:
        fs = score_biome(temp, humid, height, p, p["forest_temp_affinity"], p["forest_humid_affinity"], p["forest_height_affinity"], p["forest_adjust"])
        ds = score_biome(temp, humid, height, p, p["desert_temp_affinity"], p["desert_humid_affinity"], p["desert_height_affinity"], p["desert_adjust"])
        ps = score_biome(temp, humid, height, p, p["plains_temp_affinity"], p["plains_humid_affinity"], p["plains_height_affinity"], p["plains_adjust"])
        ims = score_biome(temp, humid, height, p, p["ice_mtn_temp_affinity"], p["ice_mtn_humid_affinity"], p["ice_mtn_height_affinity"], p["ice_mtn_adjust"])
        hms = score_biome(temp, humid, height, p, p["humid_mtn_temp_affinity"], p["humid_mtn_humid_affinity"], p["humid_mtn_height_affinity"], p["humid_mtn_adjust"])
        vms = score_biome(temp, humid, height, p, p["classic_mtn_temp_affinity"], p["classic_mtn_humid_affinity"], p["classic_mtn_height_affinity"], p["classic_mtn_adjust"])

        best = max(fs, ds, ps, ims, hms, vms)
        if best == ims: biome = BIOME_ICE_MTN
        elif best == hms: biome = BIOME_HUMID_MTN
        elif best == vms: biome = BIOME_CLASSIC_MTN
        elif best == fs: biome = BIOME_FOREST
        elif best == ds: biome = BIOME_DESERT
        else: biome = BIOME_PLAINS

    d_sea = height - p["sea_level"]
    d_cold = abs(temp - eff_glacier)
    g = max(0.0, min(min(d_sea, d_cold) / p["biome_blend_distance"], 1.0))
    return biome, g

@dataclass
class ColumnResult:
    height: float
    biome: int
    water_surface: int
    under_biome: int = 0

def folly_contribution(wx, wy, scale, amplitude, bias, t, seed):
    raw = perlin(float(wx), float(wy), scale, 1, seed)
    n = (raw + 1.0) * 0.5
    m = max(n - bias, 0.0)
    return m * amplitude * t

def compute_column(wx, wy, p, seed):
    height = max(1.0, min(base_height(wx, wy, p, seed), p["max_height"]))
    if height >= p["sea_level"]:
        t = max(0.0, min((height - p["sea_level"]) / 20.0, 1.0))
        fs = seed + N_MOUNTAIN_FOLLY * 7919
        c1 = folly_contribution(wx, wy, p["mountain_folly_scale"], p["mountain_folly_amplitude"], p["mountain_folly_bias"], t, fs)
        c2 = folly_contribution(wx, wy, p["mountain_folly_scale"] * 0.8, p["mountain_folly_amplitude"] * 0.8, p["mountain_folly_bias"], t, fs + 1)
        c3 = folly_contribution(wx, wy, p["mountain_folly_scale"] * 1.1, p["mountain_folly_amplitude"] * 1.1, p["mountain_folly_bias"], t, fs + 2)
        height += max(c1, c2, c3)
        height = max(1.0, min(height, p["max_height"]))

    is_sea = height < p["sea_level"]

    temp = max(0.0, min((perlin(float(wx), float(wy), p["temp_scale"], 1,
        seed + N_TEMPERATURE * 7919) + 1) * 0.5, 1.0))
    humid = max(0.0, min((perlin(float(wx), float(wy), p["humid_scale"], 1,
        seed + N_HUMIDITY * 7919) + 1) * 0.5, 1.0))
    temp = max(0.0, min(temp + perlin(float(wx), float(wy), p["temp_perturb_scale"], 1,
        seed + N_TEMP_PERTURB * 7919) * p["temp_perturb_amplitude"], 1.0))
    humid = max(0.0, min(humid + perlin(float(wx), float(wy), p["humid_perturb_scale"], 1,
        seed + N_HUMID_PERTURB * 7919) * p["humid_perturb_amplitude"], 1.0))

    biome = 0
    ws = 0
    under = BIOME_PLAINS

    if is_sea:
        sf = perlin(float(wx), float(wy), p["sea_floor_scale"], 1,
                    seed + N_SEA_FLOOR * 7919) * p["sea_floor_amplitude"]
        raw_depth = p["sea_level"] - height
        eff_depth = raw_depth if raw_depth <= p["sea_max_depth"] else p["sea_max_depth"] + (raw_depth - p["sea_max_depth"]) * p["sea_depth_slope"]
        height = p["sea_level"] - eff_depth + sf
        height = max(1.0, min(height, p["max_height"]))
        ws = int(min(max(p["sea_level"], 0), 255))
        biome = BIOME_PLAINS
    else:
        eff_glacier = p["glacier_threshold"] + p["ice_age_factor"] * (1.0 - p["glacier_threshold"])

        fs = score_biome(temp, humid, height, p, p["forest_temp_affinity"], p["forest_humid_affinity"], p["forest_height_affinity"], p["forest_adjust"])
        ds = score_biome(temp, humid, height, p, p["desert_temp_affinity"], p["desert_humid_affinity"], p["desert_height_affinity"], p["desert_adjust"])
        ps = score_biome(temp, humid, height, p, p["plains_temp_affinity"], p["plains_humid_affinity"], p["plains_height_affinity"], p["plains_adjust"])
        ims = score_biome(temp, humid, height, p, p["ice_mtn_temp_affinity"], p["ice_mtn_humid_affinity"], p["ice_mtn_height_affinity"], p["ice_mtn_adjust"])
        hms = score_biome(temp, humid, height, p, p["humid_mtn_temp_affinity"], p["humid_mtn_humid_affinity"], p["humid_mtn_height_affinity"], p["humid_mtn_adjust"])
        vms = score_biome(temp, humid, height, p, p["classic_mtn_temp_affinity"], p["classic_mtn_humid_affinity"], p["classic_mtn_height_affinity"], p["classic_mtn_adjust"])

        best = max(fs, ds, ps, ims, hms, vms)
        if best == ims: under = BIOME_ICE_MTN
        elif best == hms: under = BIOME_HUMID_MTN
        elif best == vms: under = BIOME_CLASSIC_MTN
        elif best == fs: under = BIOME_FOREST
        elif best == ds: under = BIOME_DESERT
        else: under = BIOME_PLAINS

        if temp < eff_glacier:
            biome = BIOME_COLDPLACE
        else:
            biome = under

        d_sea = height - p["sea_level"]
        d_cold = abs(temp - eff_glacier)
        g = max(0.0, min(min(d_sea, d_cold) / p["biome_blend_distance"], 1.0))

        if biome == BIOME_ICE_MTN:
            mtn_grad = max(0.0, min((height - p["mountain_start"]) / p["biome_blend_distance"], 1.0))
            height += perlin(float(wx), float(wy), p["mountain_detail_scale"], 2,
                seed + N_MOUNTAIN_DETAIL * 7919) * p["mountain_detail_amplitude"] * mtn_grad
            mtn_factor = max(0.0, min((height - p["mountain_start"]) / (p["max_height"] - p["mountain_start"]), 1.0))
            lift = math.sin(mtn_factor * math.pi * 0.5)
            height += perlin(float(wx), float(wy), p["mountain_lift_scale"], 1,
                seed + N_MOUNTAIN_DETAIL * 7919 + 31337) * p["ice_mtn_peak_amplitude"] * lift
            rn = perlin(float(wx), float(wy), p["mountain_rough_scale"], 1, seed + N_MOUNTAIN_ROUGH * 7919)
            if rn > p["mountain_rough_threshold"]:
                rough_mask = (rn - p["mountain_rough_threshold"]) / (1.0 - p["mountain_rough_threshold"])
                d1 = perlin(float(wx), float(wy), p["mountain_rough_detail_scale"], 1, seed + N_MOUNTAIN_ROUGH * 7919 + 7901)
                d2 = perlin(float(wx), float(wy), p["mountain_rough_detail_scale"] * 2.0, 1, seed + N_MOUNTAIN_ROUGH * 7919 + 7907)
                height += (d1 * 0.6 + d2 * 0.4) * p["mountain_rough_amplitude"] * rough_mask * mtn_factor
        elif biome == BIOME_CLASSIC_MTN:
            mtn_grad = max(0.0, min((height - p["mountain_start"]) / p["biome_blend_distance"], 1.0))
            height += perlin(float(wx), float(wy), p["mountain_detail_scale"], 2,
                seed + N_MOUNTAIN_DETAIL * 7919) * p["mountain_detail_amplitude"] * mtn_grad
            mtn_factor = max(0.0, min((height - p["mountain_start"]) / (p["max_height"] - p["mountain_start"]), 1.0))
            lift = math.sin(mtn_factor * math.pi * 0.5)
            height += perlin(float(wx), float(wy), p["mountain_lift_scale"], 1,
                seed + N_MOUNTAIN_DETAIL * 7919 + 31337) * p["classic_mtn_lift_amplitude"] * lift
            rn = perlin(float(wx), float(wy), p["mountain_rough_scale"], 1, seed + N_MOUNTAIN_ROUGH * 7919)
            if rn > p["mountain_rough_threshold"]:
                rough_mask = (rn - p["mountain_rough_threshold"]) / (1.0 - p["mountain_rough_threshold"])
                d1 = perlin(float(wx), float(wy), p["mountain_rough_detail_scale"], 1, seed + N_MOUNTAIN_ROUGH * 7919 + 7901)
                d2 = perlin(float(wx), float(wy), p["mountain_rough_detail_scale"] * 2.0, 1, seed + N_MOUNTAIN_ROUGH * 7919 + 7907)
                height += (d1 * 0.6 + d2 * 0.4) * p["mountain_rough_amplitude"] * rough_mask * mtn_factor
        elif biome == BIOME_HUMID_MTN:
            mtn_grad = max(0.0, min((height - p["mountain_start"]) / p["biome_blend_distance"], 1.0))
            height += perlin(float(wx), float(wy), p["mountain_detail_scale"], 2,
                seed + N_MOUNTAIN_DETAIL * 7919) * p["mountain_detail_amplitude"] * 0.5 * mtn_grad
            height += abs(perlin(float(wx), float(wy), p["humid_mtn_hill_scale"], 1,
                seed + N_PLAINS_HILL * 7919)) * p["humid_mtn_hill_amplitude"] * g
        elif biome == BIOME_PLAINS:
            height += abs(perlin(float(wx), float(wy), p["plains_hill_scale"], 1,
                seed + N_PLAINS_HILL * 7919)) * p["plains_hill_amplitude"] * g
        elif biome == BIOME_DESERT:
            height += abs(perlin(float(wx), float(wy), p["desert_dune_scale"], 1,
                seed + N_DESERT_DUNE * 7919)) * p["desert_dune_amplitude"] * g

        height = max(1.0, min(height, p["max_height"]))

    height += perlin(float(wx), float(wy), p["meso_scale"], p["meso_octaves"],
        seed + N_MESO * 7919, p["meso_persistence"], p["meso_lacunarity"]) * p["meso_amplitude"]
    height += perlin(float(wx), float(wy), p["micro_scale"], p["micro_octaves"],
        seed + N_MICRO * 7919, p["micro_persistence"], p["micro_lacunarity"]) * p["micro_amplitude"]
    height = max(1.0, min(height, p["max_height"]))

    return ColumnResult(height=height, biome=biome, water_surface=ws, under_biome=under if biome == BIOME_COLDPLACE else biome)

def _base_biome_color(biome, height, p):
    if biome == BIOME_PLAINS: return (34, 139, 34)
    if biome == BIOME_DESERT: return (210, 180, 100)
    if biome == BIOME_FOREST: return (20, 100, 20)
    if biome == BIOME_COLDPLACE: return (200, 220, 240)
    if biome == BIOME_ICE_MTN:
        if height > p["mountain_snow_threshold"]: return (240, 240, 240)
        if height > p["mountain_rock_threshold"]: return (160, 160, 160)
        return (80, 130, 60)
    if biome == BIOME_HUMID_MTN: return (20, 180, 20)
    if biome == BIOME_CLASSIC_MTN:
        if height > p["mountain_snow_threshold"]: return (240, 240, 240)
        if height > p["mountain_rock_threshold"]: return (160, 160, 160)
        return (34, 139, 34)
    return (255, 0, 255)

def _blend(c1, c2, t):
    return tuple(int(a + (b - a) * t) for a, b in zip(c1, c2))

def biome_color(biome, height, water, under_biome, p):
    cold_color = (200, 220, 240)
    if water > 0:
        if biome == BIOME_COLDPLACE:
            return _blend((30, 100, 200), cold_color, 0.45)
        return (30, 100, 200)
    if height < p["sea_level"]:
        return (30, 80, 160)
    base = _base_biome_color(biome, height, p)
    if biome == BIOME_COLDPLACE:
        under_c = _base_biome_color(under_biome, height, p)
        return _blend(under_c, cold_color, 0.45)
    return base

def generate_map(size, p, seed):
    img = Image.new("RGB", (size, size))
    pix = img.load()
    wr = 32768
    step = wr / size
    print(f"{size}x{size} seed={seed} ...")
    for py in range(size):
        if py % 128 == 0:
            print(f"  line {py}/{size}")
        wy = int((py - size / 2) * step)
        for px in range(size):
            wx = int((px - size / 2) * step)
            col = compute_column(wx, wy, p, seed)
            pix[px, py] = biome_color(col.biome, col.height, col.water_surface, col.under_biome, p)
    return img

def _gray(v):
    v = max(0, min(255, int(v)))
    return (v, v, v)

def _heatmap(t):
    t = max(0.0, min(1.0, t))
    if t < 0.5:
        r = 0; g = int(255 * (t * 2)); b = int(255 * (1 - t * 2))
    else:
        r = int(255 * ((t - 0.5) * 2)); g = int(255 * (1 - (t - 0.5) * 2)); b = 0
    return (r, g, b)

def _humidmap(h):
    h = max(0.0, min(1.0, h))
    r = int(180 * (1 - h)); g = int(140 * (1 - h)); b = int(80 + 175 * h)
    return (r, g, b)

def _build_grid(size, p, seed):
    wr = 32768
    step = wr / size
    grid = [[None]*size for _ in range(size)]
    for py in range(size):
        if py % 128 == 0:
            print(f"  grid {py}/{size}")
        wy = int((py - size / 2) * step)
        for px in range(size):
            wx = int((px - size / 2) * step)
            grid[py][px] = compute_column(wx, wy, p, seed)
    return grid, step

def _render_biomes(grid, p, size):
    img = Image.new("RGB", (size, size))
    pix = img.load()
    for py in range(size):
        for px in range(size):
            col = grid[py][px]
            pix[px, py] = biome_color(col.biome, col.height, col.water_surface, col.under_biome, p)
    return img

def _render_height(grid, p, size):
    img = Image.new("RGB", (size, size))
    pix = img.load()
    for py in range(size):
        for px in range(size):
            h = grid[py][px].height
            v = (h / p["max_height"]) * 255
            pix[px, py] = _gray(v)
    return img

def _render_temperature(size, p, seed):
    img = Image.new("RGB", (size, size))
    pix = img.load()
    wr = 32768
    step = wr / size
    for py in range(size):
        for px in range(size):
            wx = int((px - size/2) * step)
            wy = int((py - size/2) * step)
            t = (perlin(float(wx), float(wy), p["temp_scale"], 1,
                seed + N_TEMPERATURE * 7919) + 1) * 0.5
            t = max(0.0, min(1.0, t + perlin(float(wx), float(wy), p["temp_perturb_scale"], 1,
                seed + N_TEMP_PERTURB * 7919) * p["temp_perturb_amplitude"]))
            pix[px, py] = _heatmap(t)
    return img

def _render_humidity(size, p, seed):
    img = Image.new("RGB", (size, size))
    pix = img.load()
    wr = 32768
    step = wr / size
    for py in range(size):
        for px in range(size):
            wx = int((px - size/2) * step)
            wy = int((py - size/2) * step)
            h = (perlin(float(wx), float(wy), p["humid_scale"], 1,
                seed + N_HUMIDITY * 7919) + 1) * 0.5
            h = max(0.0, min(1.0, h + perlin(float(wx), float(wy), p["humid_perturb_scale"], 1,
                seed + N_HUMID_PERTURB * 7919) * p["humid_perturb_amplitude"]))
            pix[px, py] = _humidmap(h)
    return img

def _render_climate(grid, size, p, seed, step):
    img = Image.new("RGB", (size, size))
    pix = img.load()
    for py in range(size):
        for px in range(size):
            t = (perlin(float(int((px - size/2)*step)), float(int((py - size/2)*step)),
                p["temp_scale"], 1, seed + N_TEMPERATURE * 7919) + 1) * 0.5
            h = (perlin(float(int((px - size/2)*step)), float(int((py - size/2)*step)),
                p["humid_scale"], 1, seed + N_HUMIDITY * 7919) + 1) * 0.5
            r = int(max(0, min(255, t * 255)))
            g = int(max(0, min(255, 128)))
            b = int(max(0, min(255, h * 255)))
            pix[px, py] = (r, g, b)
    return img

def _render_mountains(grid, p, size):
    img = Image.new("RGB", (size, size))
    pix = img.load()
    mountain_colors = {
        BIOME_ICE_MTN: (80, 130, 60),
        BIOME_HUMID_MTN: (20, 180, 20),
        BIOME_CLASSIC_MTN: (34, 139, 34),
    }
    for py in range(size):
        for px in range(size):
            col = grid[py][px]
            if col.biome in mountain_colors:
                c = mountain_colors[col.biome]
                if col.height > p["mountain_snow_threshold"]:
                    c = (240, 240, 240)
                elif col.height > p["mountain_rock_threshold"]:
                    c = (160, 160, 160)
                pix[px, py] = c
            else:
                pix[px, py] = (20, 20, 30)
    return img

def _render_lakes(grid, size, p, seed, step):
    img = Image.new("RGB", (size, size))
    pix = img.load()
    hmap = [[grid[y][x].height for x in range(size)] for y in range(size)]

    depressions = []
    for py in range(2, size-2):
        for px in range(2, size-2):
            h = hmap[py][px]
            if h < p["sea_level"] or h >= p["max_height"] - 5: continue
            wx = int((px - size/2) * step)
            wy = int((py - size/2) * step)
            t = (perlin(float(wx), float(wy), p["temp_scale"], 1,
                seed + N_TEMPERATURE * 7919) + 1) * 0.5
            if t > 0.7: continue
            is_min = True
            for dy in range(-1, 2):
                for dx in range(-1, 2):
                    if dx == 0 and dy == 0: continue
                    if hmap[py+dy][px+dx] <= h:
                        is_min = False
                        break
                if not is_min: break
            if is_min:
                depressions.append((h, px, py))
    depressions.sort(key=lambda x: x[0])

    lake_map = [[0]*size for _ in range(size)]
    visited = [[False]*size for _ in range(size)]
    for _, sx, sy in depressions:
        if visited[sy][sx]: continue
        floor_h = hmap[sy][sx]
        spill_h = floor_h
        q = deque()
        q.append((sx, sy))
        visited[sy][sx] = True
        basin = [(sx, sy)]
        while q:
            cx, cy = q.popleft()
            for dy in range(-1, 2):
                for dx in range(-1, 2):
                    if dx == 0 and dy == 0: continue
                    nx, ny = cx+dx, cy+dy
                    if nx < 0 or nx >= size or ny < 0 or ny >= size: continue
                    if visited[ny][nx]: continue
                    nh = hmap[ny][nx]
                    if nh > floor_h + 8: continue
                    visited[ny][nx] = True
                    basin.append((nx, ny))
                    q.append((nx, ny))
                    if nh > spill_h and nh <= floor_h + 8:
                        spill_h = nh
        fill_level = floor_h + (spill_h - floor_h) * 0.75
        if fill_level <= floor_h + 0.3: continue
        if len(basin) < 5: continue
        for bx, by in basin:
            if hmap[by][bx] <= fill_level:
                lake_map[by][bx] = 1

    for py in range(size):
        for px in range(size):
            if lake_map[py][px]:
                pix[px, py] = (30, 100, 200)
            else:
                pix[px, py] = (20, 20, 30)
    return img

def _render_rivers(grid, size, p, seed, step):
    img = Image.new("RGB", (size, size))
    pix = img.load()
    hmap = [[grid[y][x].height for x in range(size)] for y in range(size)]

    flow = [[0]*size for _ in range(size)]
    flow_dir = [[-1]*size for _ in range(size)]

    for py in range(size):
        for px in range(size):
            h = hmap[py][px]
            wx = int((px - size/2) * step)
            wy = int((py - size/2) * step)
            t = (perlin(float(wx), float(wy), p["temp_scale"], 1,
                seed + N_TEMPERATURE * 7919) + 1) * 0.5
            hm = (perlin(float(wx), float(wy), p["humid_scale"], 1,
                seed + N_HUMIDITY * 7919) + 1) * 0.5
            t = max(0.0, min(1.0, t + perlin(float(wx), float(wy), p["temp_perturb_scale"], 1,
                seed + N_TEMP_PERTURB * 7919) * p["temp_perturb_amplitude"]))
            hm = max(0.0, min(1.0, hm + perlin(float(wx), float(wy), p["humid_perturb_scale"], 1,
                seed + N_HUMID_PERTURB * 7919) * p["humid_perturb_amplitude"]))
            rain = 1.0
            if hm < 0.35 and t > 0.65:
                rain = 0.0
            else:
                rain = (0.1 + 1.4 * hm) * max(1.0 - t * 0.6, 0.05)
            alt_norm = max(0.0, min(1.0, h / p["max_height"]))
            alt_factor = 1.0
            if alt_norm < 0.15:
                alt_factor = alt_norm / 0.15
            alt_factor *= (1.0 - alt_norm * 0.7)
            rain *= alt_factor
            flow[py][px] = max(0, round(rain * 10))

    D8_DX = [1, 1, 0, -1, -1, -1, 0, 1]
    D8_DY = [0, -1, -1, -1, 0, 1, 1, 1]

    for py in range(size):
        for px in range(size):
            h = hmap[py][px]
            best_dir = -1
            best_h = h
            for d in range(8):
                nx, ny = px + D8_DX[d], py + D8_DY[d]
                if 0 <= nx < size and 0 <= ny < size:
                    nh = hmap[ny][nx]
                    if nh < best_h:
                        best_h = nh
                        best_dir = d
            flow_dir[py][px] = best_dir

    pixels = []
    for py in range(size):
        for px in range(size):
            pixels.append((hmap[py][px], px, py))
    pixels.sort(key=lambda x: -x[0])

    for _, px, py in pixels:
        d = flow_dir[py][px]
        if d < 0: continue
        nx, ny = px + D8_DX[d], py + D8_DY[d]
        if 0 <= nx < size and 0 <= ny < size:
            flow[ny][nx] += flow[py][px]

    max_flow = 1
    for y in range(size):
        for x in range(size):
            if flow[y][x] > max_flow:
                max_flow = flow[y][x]

    for py in range(size):
        for px in range(size):
            f = flow[py][px]
            if f <= 0:
                pix[px, py] = (20, 20, 30)
            else:
                t = min(1.0, f / max_flow)
                r = int(30 + 30 * t)
                g = int(60 + 80 * (1-t))
                b = int(140 + 60 * (1-t))
                pix[px, py] = (r, g, b)
                if t > 0.01:
                    w = 1 if t < 0.1 else (2 if t < 0.3 else 3)
                    for dy in range(-w, w+1):
                        for dx in range(-w, w+1):
                            if dx == 0 and dy == 0: continue
                            nx, ny = px+dx, py+dy
                            if 0 <= nx < size and 0 <= ny < size:
                                cr, cg, cb = pix[nx, ny]
                                if cr == 20 and cg == 20 and cb == 30:
                                    pix[nx, ny] = (r+10, g+20, b+10)
    return img

_GRID_NEEDED = {"biomes", "height", "mountains", "lakes", "rivers", "climate"}
_NOGRID_MODES = {"temperature", "humidity"}

def generate_layer(size, p, seed, mode):
    print(f"{size}x{size} mode={mode} seed={seed} ...")
    grid, step = _build_grid(size, p, seed)
    return _render_mode(grid, step, size, p, seed, mode)

def _render_mode(grid, step, size, p, seed, mode):
    if mode == "biomes":     return _render_biomes(grid, p, size)
    if mode == "height":     return _render_height(grid, p, size)
    if mode == "temperature": return _render_temperature(size, p, seed)
    if mode == "humidity":    return _render_humidity(size, p, seed)
    if mode == "climate":    return _render_climate(grid, size, p, seed, step)
    if mode == "mountains":  return _render_mountains(grid, p, size)
    if mode == "lakes":      return _render_lakes(grid, size, p, seed, step)
    if mode == "rivers":     return _render_rivers(grid, size, p, seed, step)
    return Image.new("RGB", (size, size))

LEGEND_ENTRIES = [
    ((34, 139, 34),   "Plaines"),
    ((210, 180, 100), "Desert"),
    ((20, 100, 20),   "Foret"),
    ((200, 220, 240), "Endroit Froid"),
    ((240, 240, 240), "Mont. Glacee (neige)"),
    ((160, 160, 160), "Mont. Glacee (roche)"),
    ((80, 130, 60),   "Mont. Glacee (pentes)"),
    ((20, 180, 20),   "Mont. Humide"),
    ((34, 139, 34),   "Montagne (pentes)"),
    ((160, 160, 160), "Montagne (roche)"),
    ((240, 240, 240), "Montagne (neige)"),
    ((30, 100, 200),  "Eau"),
]

def add_legend(img, entries):
    draw = ImageDraw.Draw(img)
    try:
        font = ImageFont.truetype("arial.ttf", 14)
    except OSError:
        try:
            font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 14)
        except OSError:
            font = ImageFont.load_default()

    swatch = 14
    pad = 8
    line_h = swatch + 6
    box_w = 200
    box_h = pad * 2 + len(entries) * line_h + 4
    x0 = img.width - box_w - 10
    y0 = 10

    draw.rectangle([x0 - 2, y0 - 2, x0 + box_w + 2, y0 + box_h + 2], fill=(0, 0, 0, 180))
    draw.rectangle([x0 - 2, y0 - 2, x0 + box_w + 2, y0 + box_h + 2], outline=(255, 255, 255))

    y = y0 + pad
    for color, label in entries:
        draw.rectangle([x0 + pad, y, x0 + pad + swatch, y + swatch], fill=color, outline=(200, 200, 200))
        draw.text((x0 + pad + swatch + 6, y - 1), label, fill=(255, 255, 255), font=font)
        y += line_h
    return img

def _draw_region_grid(img, size):
    REGION_BLOCKS = 512
    wr = 32768
    step = wr / size
    px_per_region = REGION_BLOCKS / step
    draw = ImageDraw.Draw(img)
    x = 0.0
    while x < size:
        ix = int(round(x))
        draw.line([(ix, 0), (ix, size-1)], fill=(255, 255, 0), width=1)
        x += px_per_region
    y = 0.0
    while y < size:
        iy = int(round(y))
        draw.line([(0, iy), (size-1, iy)], fill=(255, 255, 0), width=1)
        y += px_per_region
    return img

LAYER_MODES = ["biomes", "height", "temperature", "humidity", "climate", "mountains", "lakes", "rivers"]

def _get_font():
    try:
        return ImageFont.truetype("arial.ttf", 16)
    except OSError:
        try:
            return ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 16)
        except OSError:
            return ImageFont.load_default()

def _label_layer(img, text):
    draw = ImageDraw.Draw(img)
    font = _get_font()
    draw.rectangle([0, 0, img.width, 22], fill=(0, 0, 0))
    draw.text((6, 3), text, fill=(255, 255, 255), font=font)
    return img

def compose_grid(images, cols=4, padding=4, bg=(0, 0, 0)):
    count = len(images)
    rows = (count + cols - 1) // cols
    w, h = images[0].size
    total_w = cols * w + (cols + 1) * padding
    total_h = rows * h + (rows + 1) * padding
    canvas = Image.new("RGB", (total_w, total_h), bg)
    for i, img in enumerate(images):
        r, c = divmod(i, cols)
        x = padding + c * (w + padding)
        y = padding + r * (h + padding)
        canvas.paste(img, (x, y))
    return canvas

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--size", type=int, default=1024)
    ap.add_argument("--seed", type=int, default=0)
    ap.add_argument("--out", type=str, default="carte.png")
    ap.add_argument("--mode", type=str, default="biomes", choices=LAYER_MODES + ["all"],
                    help="Which layer to render (or 'all' for every layer in one image)")
    ap.add_argument("--cols", type=int, default=4, help="Columns in grid when --mode all")
    a = ap.parse_args()

    if a.mode == "all":
        print(f"{a.size}x{a.size} seed={a.seed} (single grid pass) ...")
        grid, step = _build_grid(a.size, PARAMS, a.seed)
        imgs = []
        for m in LAYER_MODES:
            img = _render_mode(grid, step, a.size, PARAMS, a.seed, m)
            _draw_region_grid(img, a.size)
            _label_layer(img, m.upper())
            if m == "biomes":
                add_legend(img, LEGEND_ENTRIES)
            imgs.append(img)
        canvas = compose_grid(imgs, cols=a.cols)
        canvas.save(a.out)
        print(f"\nSaved: {a.out} ({canvas.width}x{canvas.height})")
    else:
        _offset_cache.clear()
        img = generate_layer(a.size, PARAMS, a.seed, a.mode)
        _draw_region_grid(img, a.size)
        if a.mode == "biomes":
            add_legend(img, LEGEND_ENTRIES)
        img.save(a.out)
        print(f"\nSaved: {a.out}")

if __name__ == "__main__":
    main()
