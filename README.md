# Nori Ray Tracer (Ongoing)

Physically-based rendering engine with Monte Carlo sampling.

## Results

### Rendering

<p align="center">
  <img src="nori_resuilt/3_rendering_using_Octree.png" height="200"/>
  <img src="nori_resuilt/2_rendering_bunny.png" height="200"/>
</p>

### Monte Carlo Sampling

#### Sphere & Hemisphere

<p align="center">
  <img src="nori_resuilt/sphereSampleDistribution.gif" height="180"/>
  <img src="nori_resuilt/hemishpereSampleDistribution.gif" height="180"/>
</p>

<p align="center">
  <img src="nori_resuilt/SpherePDF.png" height="180"/>
  <img src="nori_resuilt/hemispherePDF.png" height="180"/>
</p>

#### Cosine-Weighted Hemisphere

<p align="center">
  <img src="nori_resuilt/CosinWeigtedSampleDistribution.png" height="180"/>
  <img src="nori_resuilt/cosinWeigtedSamplingPDF.png" height="180"/>
</p>

#### Tent Distribution

<p align="center">
  <img src="nori_resuilt/squareToTent_result.png" height="180"/>
  <img src="nori_resuilt/SquareToTentPDF_result.png" height="180"/>
</p>

#### Uniform Disk

<p align="center">
  <img src="nori_resuilt/SqureToUniformDisk.png" height="180"/>
  <img src="nori_resuilt/SquareToUnformDiskPDF.png" height="180"/>
</p>

## Features

- Ray tracing with Octree acceleration
- Monte Carlo sampling (Tent, Disk, Sphere, Hemisphere, Beckmann)
- Chi-squared statistical validation
- Surface normal visualization

## Build

### TBB Configuration Fix

**CMakeLists.txt**: Move `find_package(TBB)` to root (lines 5-8)

**ext/CMakeLists.txt**: Remove duplicate `find_package` (lines 131-134, 184)

**src/main.cpp**:
```cpp
// Line 30
#include <tbb/global_control.h>

// Line 91
tbb::global_control

// Line 244
std::thread::hardware_concurrency()
```

### Build Commands

```bash
mkdir build && cd build
cmake ..
make -j
```

## Run

```bash
./nori scene.xml
./warptest
```

---

Advanced Computer Graphics Project
