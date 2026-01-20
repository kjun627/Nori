# Nori Ray Tracer (Ongoing)

Physically-based rendering engine with Monte Carlo sampling.

---

<p align="center">
  <img src="nori_resuilt/scene.png" width="700"/>
  <br>
  <em>Custom Scene - Microfacet materials with various roughness and colors</em>
</p>

---

## Rendering Results

### Path Tracing (Global Illumination)

<table>
<tr>
<th></th>
<th>Cornell Box</th>
<th>Veach MIS Scene</th>
<th>Table Scene</th>
</tr>
<tr>
<td><b>BSDF Sampling</b><br><sub>(Brute Force)</sub></td>
<td><img src="nori_resuilt/burute_path_cbox.png" height="180"/></td>
<td><img src="nori_resuilt/burute_veach.png" height="180"/></td>
<td><img src="nori_resuilt/burute_path.png" height="180"/></td>
</tr>
<tr>
<td><b>Next Event Estimation</b><br><sub>(Emitter Sampling)</sub></td>
<td><img src="nori_resuilt/cbox_ems.png" height="180"/></td>
<td><img src="nori_resuilt/veach_ems.png" height="180"/></td>
<td><img src="nori_resuilt/table_ems.png" height="180"/></td>
</tr>
</table>

### Ajax Bust Series

| Surface Normal | Simple Integrator | Ambient Occlusion |
|:--------------:|:-----------------:|:-----------------:|
| <img src="nori_resuilt/3_rendering_using_Octree.png" height="180"/> | <img src="nori_resuilt/SimpleWithShadow.png" height="180"/> | <img src="nori_resuilt/aoResult.png" height="180"/> |

| Microfacet Smooth (α=0.05) | Microfacet Rough (α=0.3) |
|:--------------------------:|:------------------------:|
| <img src="nori_resuilt/ajax-smooth.png" height="180"/> | <img src="nori_resuilt/ajax-rough.png" height="180"/> |

### Cornell Box & EPFL Logo

| Distribution Ray Tracing | Whitted-Style | Diffuse | Dielectric |
|:------------------------:|:-------------:|:-------:|:----------:|
| <img src="nori_resuilt/cbox-distributed.png" height="180"/> | <img src="nori_resuilt/cbox-whitted.png" height="180"/> | <img src="nori_resuilt/logo-diffuse.png" height="180"/> | <img src="nori_resuilt/logo_dielectric.png" height="180"/> |

---

## Monte Carlo Sampling

<details>
<summary><b>Sphere & Hemisphere Sampling</b></summary>
<br>

| | Sphere | Hemisphere | Cosine-Weighted |
|:-:|:------:|:----------:|:---------------:|
| **Distribution** | <img src="nori_resuilt/sphereSampleDistribution.gif" height="150"/> | <img src="nori_resuilt/hemishpereSampleDistribution.gif" height="150"/> | <img src="nori_resuilt/CosinWeigtedSampleDistribution.png" height="150"/> |
| **χ² Test** | <img src="nori_resuilt/SpherePDF.png" height="150"/> | <img src="nori_resuilt/hemispherePDF.png" height="150"/> | <img src="nori_resuilt/cosinWeigtedSamplingPDF.png" height="150"/> |

</details>

<details>
<summary><b>2D Sampling (Tent, Disk, Beckmann)</b></summary>
<br>

| | Tent | Uniform Disk | Beckmann |
|:-:|:----:|:------------:|:--------:|
| **Distribution** | <img src="nori_resuilt/squareToTent_result.png" height="150"/> | <img src="nori_resuilt/SqureToUniformDisk.png" height="150"/> | <img src="nori_resuilt/BeckmannSample.gif" height="150"/> |
| **χ² Test** | <img src="nori_resuilt/SquareToTentPDF_result.png" height="150"/> | <img src="nori_resuilt/SquareToUnformDiskPDF.png" height="150"/> | <img src="nori_resuilt/BeckmannPDF.png" height="150"/> |

</details>

<details>
<summary><b>Hierarchical Sample Warping (Environment Map)</b></summary>
<br>

| 2×2 Test | Light Probe |
|:--------:|:-----------:|
| <img src="nori_resuilt/test_2x2_original.png" height="120"/> | <img src="nori_resuilt/lightPRob.png" height="120"/> |
| <img src="nori_resuilt/Hierarchical SampleWarping2x2Sample.png" height="120"/> | <img src="nori_resuilt/ligtpropSample.png" height="120"/> |
| <img src="nori_resuilt/Hierarchical SampleWarping2x2PDF.png" height="120"/> | <img src="nori_resuilt/lightProbPDF.png" height="120"/> |

</details>

---

## Features

| Category | Implementations |
|:---------|:----------------|
| **Acceleration** | Octree spatial partitioning |
| **Integrators** | Normal, Simple, AO, Distribution, Whitted, **Path Tracing**, **NEE** |
| **BSDFs** | Diffuse, Mirror, Dielectric, Microfacet |
| **Sampling** | Tent, Disk, Sphere, Hemisphere, Cosine-weighted, Beckmann |
| **Advanced** | Hierarchical mipmap importance sampling, Russian Roulette |

---

## Build

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j
```

<details>
<summary><b>TBB Configuration Fix (if needed)</b></summary>

**CMakeLists.txt**: Move `find_package(TBB)` to root (lines 5-8)

**ext/CMakeLists.txt**: Remove duplicate `find_package` (lines 131-134, 184)

**src/main.cpp**:
```cpp
#include <tbb/global_control.h>  // Line 30
tbb::global_control              // Line 91
std::thread::hardware_concurrency()  // Line 244
```

</details>

## Run

```bash
./nori scene.xml
./warptest
```

---

<p align="center">
  <b>Advanced Computer Graphics Project</b>
</p>
