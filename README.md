Advanced Computer Graphics — Homeworks
======================================

Student name:

Sciper number:


## Build status

**Insert your build badge URL here**

## Homework results

| Homework   |  Links
| ---------: | ---------------------------------------------
| 1          | [report.html](results/homework-1/report.html)
| 2          | [report.html](results/homework-2/report.html)
| 3          | [report.html](results/homework-3/report.html)
| 4          | [report.html](results/homework-4/report.html)
| 5          | [report.html](results/homework-5/report.html)


## Featured result

Feel free to show off your best render here!


## Update Dependency
CMakeLists.txt:5-8	find_package(TBB)를 메인으로 이동
ext/CMakeLists.txt:131-134	중복 find_package 제거, TBB::tbb에서 include 경로 추출
ext/CMakeLists.txt:184	로컬 경로 덮어쓰기 제거
src/main.cpp:30	task_scheduler_init.h → global_control.h
src/main.cpp:91	task_scheduler_init → global_control
src/main.cpp:244	::automatic → std::thread::hardware_concurrency(