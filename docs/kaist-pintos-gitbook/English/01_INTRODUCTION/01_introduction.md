# 01_introduction

# Introduction

Welcome to KAIST CS330's Pintos project.
In this semester, we will use modified version of the pintos that is specially
tailored for KAIST's CS330 class.

Please notice that since the requirements are somewhat differed from the
vanilla one, **YOU SHOULD FOLLOW THE INSTRUCTION OF THIS MANUAL FIRST.** 
Also, you can refer the **YOUR OWN** previous pintos project, but **you need to**​**reimplemented some of the requirements according to the updated one**.

All the changes lead instructors to modify the manual. Although instructors
tried to synchronize the manual with modified version, there may exist
unsynchronized sections. If you find them, please notice to instructors.

**YOU MUST READ**​**[Legal and Ethical Issues](01_INTRODUCTION/04_legal_and_ethical_issues.md)**​ ** SECTION.**

The pintos-kaist is a simple operating system framework for the x86-64
architecture. This project forked from the pintos project from the stanford
university. Pintos (we will use Pintos rather than pintos-kaist from now)
supports kernel threads, loading and running user programs,
and a file system, but it implements all of these in a very simple way. In the
Pintos projects, you and your project team will strengthen its support in all
three of these areas. You will also add a virtual memory implementation. Pintos
could, theoretically, run on a regular x86-64 PC. Unfortunately, it is
impractical to supply every CS330 student a dedicated PC for use with Pintos.
Therefore, we will run Pintos projects in a system simulator, that is, a program
that simulates an x86-64 CPU and its peripheral devices accurately enough that
unmodified operating systems and software can run under it. In class we will use
the **QEMU** simulators. These projects are hard. CS330 has a reputation of
taking a lot of time, and deservedly so. We will do what we can to reduce the
workload, such as providing a lot of support material, but there is plenty of
hard work that needs to be done. We welcome your feedback. If you have
suggestions on how we can reduce the unnecessary overhead of assignments,
cutting them down to the important underlying issues, please let us know. This
chapter explains how to get started working with Pintos. You should read the
entire chapter before you start work on any of the projects.
