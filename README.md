# 🖥️ Mini OS Kernel Simulator (C++17)

A fully functional **Mini Operating System Kernel Simulator** built using **C++ (OOP + OS concepts)**.  
This project demonstrates **process scheduling, memory management, and a hierarchical file system**, all accessible through a custom shell interface.

---

## 🚀 Features

### 🔹 **1. Process Management (PCB Simulation)**
Each process contains:
- PID  
- Name  
- Arrival Time  
- Burst Time  
- Remaining Time  
- Priority  
- Start / Completion Time  
- Waiting & Turnaround Time  

### 🔹 **2. CPU Scheduling Algorithms**
Implemented using **polymorphism**:
- **FCFS** (First Come First Serve)  
- **SJF** (Shortest Job First, Non-Preemptive)  
- **Priority Scheduling (Non-Preemptive)**  
- **Round Robin (Preemptive)**  

📌 Outputs:
- ASCII **Gantt Chart**
- Waiting Time  
- Turnaround Time  
- Average metrics  

---

## 🔹 **3. Memory Management**
Simulates contiguous memory allocation:
- **First-Fit Allocation**
- **Best-Fit Allocation**
- Block splitting
- Block coalescing (merge adjacent free blocks)
- Fragmentation statistics
- Memory Map visualization

---

## 🔹 **4. Simple File System**
Implements a hierarchical directory structure:
- `mkdir <name>`
- `touch <filename>`
- `ls`
- `cd <dir>`
- `pwd`
- `rm <name>`

Directory tree is maintained using a `FSNode` structure with parent pointers and child maps.

---

## 🔹 **5. Interactive Shell (CLI)**
Commands supported:

