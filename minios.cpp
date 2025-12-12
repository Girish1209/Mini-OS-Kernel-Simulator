// minios.cpp
// Mini Operating System Kernel Simulator (OOPS + OS) in C++17
// Features: Process/PCB, multiple scheduling algorithms, memory manager, simple filesystem, shell CLI.
//
// Compile: g++ -std=c++17 minios.cpp -o minios
// Run: ./minios

#include <bits/stdc++.h>
using namespace std;

/* ============================
   Utilities
   ============================ */
static int GLOBAL_PID = 1;

string repeat(char c, int n) {
    return string(max(0, n), c);
}

/* ============================
   Process & PCB
   ============================ */

enum class ProcessState { NEW, READY, RUNNING, WAITING, TERMINATED };

struct Process {
    int pid;
    string name;
    int arrival_time;
    int burst_time;
    int remaining_time; // for preemptive algorithms
    int priority;
    ProcessState state;

    // For metrics after scheduling
    int start_time = -1;
    int completion_time = -1;
    int waiting_time = 0;
    int turnaround_time = 0;

    Process(string name_, int arrival_, int burst_, int priority_ = 0)
        : pid(GLOBAL_PID++), name(name_), arrival_time(arrival_), burst_time(burst_),
          remaining_time(burst_), priority(priority_), state(ProcessState::NEW) {}

    // Resets runtime-only values so the same process list can be re-run with another scheduler
    void reset_runtime() {
        remaining_time = burst_time;
        state = ProcessState::NEW;
        start_time = -1;
        completion_time = -1;
        waiting_time = 0;
        turnaround_time = 0;
    }
};

/* ============================
   Scheduler (OOP polymorphism)
   ============================ */

class Scheduler {
public:
    virtual ~Scheduler() {}
    // Accepts vector of processes by value (copy) so scheduling does not mutate user's originals
    virtual void schedule(vector<Process> procs) = 0;

protected:
    // Print results: Gantt chart and metrics
    void print_metrics_and_gantt(const vector<pair<int,int>>& gantt, const vector<Process>& procs) {
        // Gantt chart: gantt is list of (pid, duration) segments in chronological order
        cout << "\n===== GANTT CHART =====\n";
        // Top bar
        cout << " ";
        for (auto &seg : gantt) {
            int pid = seg.first;
            int dur = seg.second;
            cout << "+" << repeat('-', max(1, dur * 2));
        }
        cout << "+\n";

        // Middle with PIDs
        cout << " ";
        for (auto &seg : gantt) {
            int pid = seg.first;
            int dur = seg.second;
            string label = "P" + to_string(pid);
            int width = max(1, dur * 2);
            int padLeft = (width - (int)label.size())/2;
            int padRight = width - (int)label.size() - padLeft;
            cout << "|"
                 << repeat(' ', padLeft)
                 << label
                 << repeat(' ', padRight);
        }
        cout << "|\n";

        // Bottom bar
        cout << " ";
        for (auto &seg : gantt) {
            int dur = seg.second;
            cout << "+" << repeat('-', max(1, dur * 2));
        }
        cout << "+\n";

        // Time line
        int time = 0;
        cout << time;
        for (auto &seg : gantt) {
            time += seg.second;
            cout << repeat(' ', max(1, seg.second*2 - (int)to_string(time).size() + 1)) << time;
        }
        cout << "\n\n";

        // Metrics
        cout << "PID\tName\tArrival\tBurst\tStart\tCompletion\tWaiting\tTurnaround\n";
        double total_wait = 0, total_tat = 0;
        for (auto &p : procs) {
            cout << p.pid << "\t" << p.name << "\t" << p.arrival_time << "\t" << p.burst_time << "\t";
            cout << (p.start_time==-1 ? -1 : p.start_time) << "\t";
            cout << (p.completion_time==-1 ? -1 : p.completion_time) << "\t\t";
            cout << p.waiting_time << "\t" << p.turnaround_time << "\n";
            total_wait += p.waiting_time;
            total_tat += p.turnaround_time;
        }
        cout << fixed << setprecision(2);
        cout << "\nAverage waiting time: " << (total_wait / procs.size()) << "\n";
        cout << "Average turnaround time: " << (total_tat / procs.size()) << "\n";
        cout << "========================\n\n";
    }
};


/* ----------------------------
   FCFS Scheduler
   ---------------------------- */
class FCFS_Scheduler : public Scheduler {
public:
    void schedule(vector<Process> procs) override {
        // sort by arrival time, then pid
        sort(procs.begin(), procs.end(), [](const Process& a, const Process& b){
            if (a.arrival_time != b.arrival_time) return a.arrival_time < b.arrival_time;
            return a.pid < b.pid;
        });

        int current_time = 0;
        vector<pair<int,int>> gantt; // (pid, duration)
        for (auto &p : procs) {
            if (current_time < p.arrival_time) {
                // idle time block
                gantt.emplace_back(0, p.arrival_time - current_time); // pid 0 reserved for idle
                current_time = p.arrival_time;
            }
            p.start_time = current_time;
            p.waiting_time = p.start_time - p.arrival_time;
            current_time += p.burst_time;
            p.completion_time = current_time;
            p.turnaround_time = p.completion_time - p.arrival_time;
            gantt.emplace_back(p.pid, p.burst_time);
        }

        print_metrics_and_gantt(gantt, procs);
    }
};


/* ----------------------------
   SJF (Non-preemptive)
   ---------------------------- */
class SJF_Scheduler : public Scheduler {
public:
    void schedule(vector<Process> procs) override {
        // will pick shortest job from ready queue when CPU free
        int n = procs.size();
        for (auto &p : procs) p.reset_runtime();

        // sort by arrival
        sort(procs.begin(), procs.end(), [](const Process& a, const Process& b){
            if (a.arrival_time != b.arrival_time) return a.arrival_time < b.arrival_time;
            return a.burst_time < b.burst_time;
        });

        int completed = 0;
        int current_time = 0;
        vector<pair<int,int>> gantt;
        vector<bool> done(n, false);

        while (completed < n) {
            // find candidates arrived and not done
            int idx = -1;
            int best_burst = INT_MAX;
            for (int i = 0; i < n; ++i) {
                if (!done[i] && procs[i].arrival_time <= current_time) {
                    if (procs[i].burst_time < best_burst) {
                        best_burst = procs[i].burst_time;
                        idx = i;
                    }
                }
            }
            if (idx == -1) {
                // idle to next arrival
                int nextArrival = INT_MAX;
                for (int i = 0; i < n; ++i) if (!done[i]) nextArrival = min(nextArrival, procs[i].arrival_time);
                gantt.emplace_back(0, nextArrival - current_time);
                current_time = nextArrival;
                continue;
            }
            auto &p = procs[idx];
            p.start_time = current_time;
            p.waiting_time = p.start_time - p.arrival_time;
            current_time += p.burst_time;
            p.completion_time = current_time;
            p.turnaround_time = p.completion_time - p.arrival_time;
            done[idx] = true;
            completed++;
            gantt.emplace_back(p.pid, p.burst_time);
        }

        print_metrics_and_gantt(gantt, procs);
    }
};


/* ----------------------------
   Priority (Non-preemptive)
   ---------------------------- */
class Priority_Scheduler : public Scheduler {
public:
    void schedule(vector<Process> procs) override {
        // lower priority number => higher priority (like 0 highest)
        int n = procs.size();
        for (auto &p : procs) p.reset_runtime();

        int completed = 0;
        int current_time = 0;
        vector<pair<int,int>> gantt;
        vector<bool> done(n, false);

        while (completed < n) {
            int idx = -1;
            int best_pr = INT_MAX;
            for (int i = 0; i < n; ++i) {
                if (!done[i] && procs[i].arrival_time <= current_time) {
                    if (procs[i].priority < best_pr) {
                        best_pr = procs[i].priority;
                        idx = i;
                    } else if (procs[i].priority == best_pr) {
                        if (procs[i].arrival_time < procs[idx].arrival_time)
                            idx = i;
                    }
                }
            }
            if (idx == -1) {
                int nextArrival = INT_MAX;
                for (int i = 0; i < n; ++i) if (!done[i]) nextArrival = min(nextArrival, procs[i].arrival_time);
                gantt.emplace_back(0, nextArrival - current_time);
                current_time = nextArrival;
                continue;
            }
            auto &p = procs[idx];
            p.start_time = current_time;
            p.waiting_time = p.start_time - p.arrival_time;
            current_time += p.burst_time;
            p.completion_time = current_time;
            p.turnaround_time = p.completion_time - p.arrival_time;
            done[idx] = true;
            completed++;
            gantt.emplace_back(p.pid, p.burst_time);
        }

        print_metrics_and_gantt(gantt, procs);
    }
};


/* ----------------------------
   Round Robin (Preemptive)
   ---------------------------- */
class RR_Scheduler : public Scheduler {
public:
    RR_Scheduler(int quantum_) : quantum(quantum_) {}
    void schedule(vector<Process> procs) override {
        int n = procs.size();
        for (auto &p : procs) p.reset_runtime();

        // sort by arrival
        sort(procs.begin(), procs.end(), [](const Process& a, const Process& b) {
            if (a.arrival_time != b.arrival_time) return a.arrival_time < b.arrival_time;
            return a.pid < b.pid;
        });

        queue<int> q; // indices in procs
        int current_time = 0;
        int idx = 0; // index for new arrivals
        vector<pair<int,int>> gantt;

        // push initial arrivals at time 0
        while (idx < n && procs[idx].arrival_time <= current_time) {
            q.push(idx++);
        }

        if (q.empty() && idx < n) {
            // idle till first arrival
            gantt.emplace_back(0, procs[idx].arrival_time - current_time);
            current_time = procs[idx].arrival_time;
            while (idx < n && procs[idx].arrival_time <= current_time) q.push(idx++);
        }

        while (!q.empty()) {
            int i = q.front(); q.pop();
            auto &p = procs[i];
            if (p.start_time == -1) p.start_time = current_time;
            int exec = min(quantum, p.remaining_time);
            // execute for 'exec'
            p.remaining_time -= exec;
            // add to gantt
            gantt.emplace_back(p.pid, exec);
            current_time += exec;

            // push newly arrived processes up to current_time
            while (idx < n && procs[idx].arrival_time <= current_time) {
                q.push(idx++);
            }

            if (p.remaining_time > 0) {
                q.push(i); // requeue
            } else {
                p.completion_time = current_time;
                p.turnaround_time = p.completion_time - p.arrival_time;
                p.waiting_time = p.turnaround_time - p.burst_time;
            }

            if (q.empty() && idx < n) {
                // idle until next arrival
                if (current_time < procs[idx].arrival_time) {
                    gantt.emplace_back(0, procs[idx].arrival_time - current_time);
                    current_time = procs[idx].arrival_time;
                }
                while (idx < n && procs[idx].arrival_time <= current_time) q.push(idx++);
            }
        }

        print_metrics_and_gantt(gantt, procs);
    }

private:
    int quantum;
};

/* ============================
   Memory Manager
   ============================ */

struct MemBlock {
    int id;
    int start;
    int size;
    bool free;
    int pid_allocated; // pid that owns the block, -1 if free

    MemBlock(int id_, int start_, int size_)
        : id(id_), start(start_), size(size_), free(true), pid_allocated(-1) {}
};

class MemoryManager {
public:
    MemoryManager(int total_size_) : total_size(total_size_) {
        blocks.emplace_back(0, 0, total_size);
        next_block_id = 1;
    }

    void reset(int new_total) {
        total_size = new_total;
        blocks.clear();
        blocks.emplace_back(0, 0, total_size);
        next_block_id = 1;
    }

    // First-fit allocation
    int allocate_first_fit(int pid, int size) {
        for (auto &b : blocks) {
            if (b.free && b.size >= size) {
                if (b.size == size) {
                    b.free = false;
                    b.pid_allocated = pid;
                    print_alloc(pid, b.start, size);
                    return b.id;
                } else {
                    // split block
                    int oldStart = b.start;
                    int oldSize = b.size;
                    b.size = size;
                    b.free = false;
                    b.pid_allocated = pid;
                    // create new free block after it
                    MemBlock nb(next_block_id++, oldStart + size, oldSize - size);
                    // insert after b
                    auto it = find_if(blocks.begin(), blocks.end(), [&](const MemBlock& mb){ return mb.id == b.id; });
                    if (it != blocks.end()) {
                        blocks.insert(it + 1, nb);
                    } else {
                        blocks.push_back(nb);
                    }
                    print_alloc(pid, oldStart, size);
                    return b.id;
                }
            }
        }
        cout << "Allocation failed: no suitable block found.\n";
        return -1;
    }

    // Best-fit allocation
    int allocate_best_fit(int pid, int size) {
        int bestIndex = -1;
        int bestSize = INT_MAX;
        for (int i = 0; i < (int)blocks.size(); ++i) {
            if (blocks[i].free && blocks[i].size >= size) {
                if (blocks[i].size < bestSize) {
                    bestSize = blocks[i].size;
                    bestIndex = i;
                }
            }
        }
        if (bestIndex == -1) {
            cout << "Allocation failed: no suitable block found.\n";
            return -1;
        }
        MemBlock &b = blocks[bestIndex];
        if (b.size == size) {
            b.free = false;
            b.pid_allocated = pid;
            print_alloc(pid, b.start, size);
            return b.id;
        } else {
            int oldStart = b.start;
            int oldSize = b.size;
            b.size = size;
            b.free = false;
            b.pid_allocated = pid;
            MemBlock nb(next_block_id++, oldStart + size, oldSize - size);
            blocks.insert(blocks.begin() + bestIndex + 1, nb);
            print_alloc(pid, oldStart, size);
            return b.id;
        }
    }

    bool deallocate_by_pid(int pid) {
        bool found = false;
        for (auto &b : blocks) {
            if (!b.free && b.pid_allocated == pid) {
                b.free = true;
                b.pid_allocated = -1;
                found = true;
            }
        }
        if (!found) {
            cout << "No blocks were allocated to PID " << pid << "\n";
            return false;
        }
        coalesce();
        cout << "Deallocated blocks for PID " << pid << "\n";
        return true;
    }

    void print_memory_map() {
        cout << "\nMemory Map (start:size) [free/pid]\n";
        for (auto &b : blocks) {
            cout << "[" << b.start << ":" << b.size << "] ";
            if (b.free) cout << "[FREE]";
            else cout << "[PID " << b.pid_allocated << "]";
            cout << "\n";
        }
        cout << "\n";
    }

    void fragmentation_stats() {
        int freeBlocks = 0, freeSize = 0;
        for (auto &b : blocks) {
            if (b.free) { freeBlocks++; freeSize += b.size; }
        }
        cout << "Total memory: " << total_size << ", Free size: " << freeSize << ", Free blocks: " << freeBlocks << "\n";
    }

private:
    int total_size;
    vector<MemBlock> blocks;
    int next_block_id;

    void coalesce() {
        // merge adjacent free blocks
        for (int i = 0; i + 1 < (int)blocks.size();) {
            if (blocks[i].free && blocks[i+1].free) {
                blocks[i].size += blocks[i+1].size;
                blocks.erase(blocks.begin() + i + 1);
            } else ++i;
        }
    }

    void print_alloc(int pid, int start, int size) {
        cout << "Allocated PID " << pid << " at [" << start << ":" << size << "]\n";
    }
};

/* ============================
   Simple File System (hierarchical)
   ============================ */

class FSNode {
public:
    string name;
    bool is_dir;
    FSNode* parent;
    map<string, FSNode*> children; // for directory
    FSNode(string name_, bool dir_, FSNode* parent_)
        : name(name_), is_dir(dir_), parent(parent_) {}

    ~FSNode() {
        for (auto &kv : children) delete kv.second;
    }
};

class SimpleFileSystem {
public:
    SimpleFileSystem() {
        root = new FSNode("/", true, nullptr);
        cwd = root;
    }
    ~SimpleFileSystem() { delete root; }

    // mkdir path (simple, only relative or single-level)
    bool mkdir(const string &dirName) {
        if (!cwd->is_dir) return false;
        if (cwd->children.count(dirName)) {
            cout << "Directory already exists: " << dirName << "\n";
            return false;
        }
        FSNode* node = new FSNode(dirName, true, cwd);
        cwd->children[dirName] = node;
        cout << "Directory created: " << dirName << "\n";
        return true;
    }

    bool touch(const string &fileName) {
        if (!cwd->is_dir) return false;
        if (cwd->children.count(fileName)) {
            cout << "File/dir already exists: " << fileName << "\n";
            return false;
        }
        FSNode* node = new FSNode(fileName, false, cwd);
        cwd->children[fileName] = node;
        cout << "File created: " << fileName << "\n";
        return true;
    }

    bool ls() {
        if (!cwd->is_dir) return false;
        for (auto &kv : cwd->children) {
            if (kv.second->is_dir) cout << kv.first << "/\t";
            else cout << kv.first << "\t";
        }
        cout << "\n";
        return true;
    }

    bool rm(const string &name) {
        if (!cwd->is_dir) return false;
        auto it = cwd->children.find(name);
        if (it == cwd->children.end()) {
            cout << "No such file/dir: " << name << "\n";
            return false;
        }
        delete it->second;
        cwd->children.erase(it);
        cout << "Removed: " << name << "\n";
        return true;
    }

    bool cd(const string &name) {
        if (name == "/") { cwd = root; return true; }
        if (name == "..") {
            if (cwd->parent) cwd = cwd->parent;
            return true;
        }
        auto it = cwd->children.find(name);
        if (it == cwd->children.end() || !it->second->is_dir) {
            cout << "No such directory: " << name << "\n";
            return false;
        }
        cwd = it->second;
        return true;
    }

    void pwd() {
        vector<string> parts;
        FSNode* cur = cwd;
        while (cur && cur != root) {
            parts.push_back(cur->name);
            cur = cur->parent;
        }
        cout << "/";
        for (int i = (int)parts.size() - 1; i >= 0; --i) {
            cout << parts[i] << (i==0? "" : "/");
        }
        cout << "\n";
    }

private:
    FSNode* root;
    FSNode* cwd;
};

/* ============================
   Shell CLI & Integration
   ============================ */

class MiniOSKernelSimulator {
public:
    MiniOSKernelSimulator() : memmgr(1000) { // default memory 1000 units
        // create default scheduler objects (we will instantiate when needed)
    }

    void run_shell() {
        cout << "=== Mini OS Kernel Simulator (C++) ===\n";
        print_help();
        string cmd;
        while (true) {
            cout << "\nminios> ";
            cout.flush();
            if (!getline(cin, cmd)) break;
            if (cmd.empty()) continue;
            vector<string> toks = split(cmd);
            if (toks.empty()) continue;

            string op = toks[0];

            if (op == "help") print_help();
            else if (op == "exit") { cout << "Exiting...\n"; break; }
            else if (op == "addproc") cmd_addproc(toks);
            else if (op == "listproc") cmd_listproc();
            else if (op == "runsched") cmd_runsched(toks);
            else if (op == "resetproc") cmd_resetproc();
            else if (op == "memmap") memmgr.print_memory_map();
            else if (op == "alloc") cmd_alloc(toks);
            else if (op == "dealloc") cmd_dealloc(toks);
            else if (op == "frag") memmgr.fragmentation_stats();
            else if (op == "mkdir") fs.mkdir(arg_or_empty(toks,1));
            else if (op == "touch") fs.touch(arg_or_empty(toks,1));
            else if (op == "ls") fs.ls();
            else if (op == "rm") fs.rm(arg_or_empty(toks,1));
            else if (op == "cd") fs.cd(arg_or_empty(toks,1));
            else if (op == "pwd") fs.pwd();
            else if (op == "clear") clear_screen();
            else if (op == "sample") cmd_sample();
            else {
                cout << "Unknown command. Type 'help' to see commands.\n";
            }
        }
    }

private:
    vector<Process> processes;
    MemoryManager memmgr;
    SimpleFileSystem fs;

    // Helpers
    static vector<string> split(const string &s) {
        vector<string> out;
        string cur;
        stringstream ss(s);
        while (ss >> cur) out.push_back(cur);
        return out;
    }
    static string arg_or_empty(const vector<string>& toks, int idx) {
        if ((int)toks.size() > idx) return toks[idx];
        return "";
    }

    void print_help() {
        cout << "Commands:\n";
        cout << "  help                 : Show this help\n";
        cout << "  addproc <name> <arrival> <burst> [priority]  : Add a process\n";
        cout << "  listproc             : List all processes\n";
        cout << "  resetproc            : Reset runtime values (to rerun scheduling)\n";
        cout << "  runsched <alg> [quantum] : Run scheduler. alg=fcfs|sjf|priority|rr\n";
        cout << "                         For rr, provide quantum (int).\n";
        cout << "  memmap               : Show memory map\n";
        cout << "  alloc <pid> <size> <policy> : Allocate memory for pid. policy=first|best\n";
        cout << "  dealloc <pid>        : Deallocate memory by pid\n";
        cout << "  frag                 : Show fragmentation stats\n";
        cout << "  mkdir <name>         : Create directory in current directory\n";
        cout << "  touch <name>         : Create file in current directory\n";
        cout << "  ls                   : List current directory\n";
        cout << "  rm <name>            : Remove file/directory\n";
        cout << "  cd <name>|/|..       : Change directory\n";
        cout << "  pwd                  : Show current directory path\n";
        cout << "  sample               : Run sample demo (adds processes & runs schedulers)\n";
        cout << "  clear                : Clear screen\n";
        cout << "  exit                 : Exit simulator\n";
    }

    void cmd_addproc(const vector<string>& toks) {
        if (toks.size() < 4) {
            cout << "Usage: addproc <name> <arrival> <burst> [priority]\n";
            return;
        }
        string name = toks[1];
        int arrival = stoi(toks[2]);
        int burst = stoi(toks[3]);
        int pr = 0;
        if (toks.size() >= 5) pr = stoi(toks[4]);
        processes.emplace_back(name, arrival, burst, pr);
        cout << "Process added: PID=" << processes.back().pid << " Name=" << name << " Arrival=" << arrival << " Burst=" << burst << " Priority=" << pr << "\n";
    }

    void cmd_listproc() {
        cout << "PID\tName\tArrival\tBurst\tPriority\n";
        for (auto &p : processes) {
            cout << p.pid << "\t" << p.name << "\t" << p.arrival_time << "\t" << p.burst_time << "\t" << p.priority << "\n";
        }
    }

    void cmd_resetproc() {
        for (auto &p : processes) p.reset_runtime();
        cout << "Process runtime values reset.\n";
    }

    void cmd_runsched(const vector<string>& toks) {
        if (toks.size() < 2) { cout << "Usage: runsched <alg> [quantum]\n"; return; }
        string alg = toks[1];
        if (processes.empty()) { cout << "No processes. Add processes with addproc.\n"; return; }

        // copy processes so schedulers don't permanently mutate original list
        vector<Process> procs_copy = processes;

        unique_ptr<Scheduler> sched;
        if (alg == "fcfs") sched = make_unique<FCFS_Scheduler>();
        else if (alg == "sjf") sched = make_unique<SJF_Scheduler>();
        else if (alg == "priority") sched = make_unique<Priority_Scheduler>();
        else if (alg == "rr") {
            if (toks.size() < 3) { cout << "Please provide quantum: runsched rr <quantum>\n"; return; }
            int q = stoi(toks[2]);
            sched = make_unique<RR_Scheduler>(q);
        } else {
            cout << "Unknown scheduler algorithm: " << alg << "\n";
            return;
        }
        cout << "Running scheduler: " << alg << " ...\n";
        sched->schedule(procs_copy);
    }

    void cmd_alloc(const vector<string>& toks) {
        if (toks.size() < 4) {
            cout << "Usage: alloc <pid> <size> <policy>\n";
            return;
        }
        int pid = stoi(toks[1]);
        int size = stoi(toks[2]);
        string policy = toks[3];
        if (policy == "first") memmgr.allocate_first_fit(pid, size);
        else if (policy == "best") memmgr.allocate_best_fit(pid, size);
        else cout << "Unknown policy. Use first or best.\n";
    }

    void cmd_dealloc(const vector<string>& toks) {
        if (toks.size() < 2) { cout << "Usage: dealloc <pid>\n"; return; }
        int pid = stoi(toks[1]);
        memmgr.deallocate_by_pid(pid);
    }

    void clear_screen() {
#ifdef _WIN32
        system("cls");
#else
        // ANSI escape
        cout << "\x1B[2J\x1B[H";
#endif
    }

    // A short sample demonstration to show expected outputs
    void cmd_sample() {
        // Clear any existing processes and memory
        processes.clear();
        GLOBAL_PID = 1;
        memmgr.reset(100);

        // Add sample processes
        processes.emplace_back("A", 0, 4, 2);
        processes.emplace_back("B", 1, 3, 1);
        processes.emplace_back("C", 2, 1, 3);
        processes.emplace_back("D", 3, 2, 2);

        cout << "Added sample processes: A(0,4), B(1,3), C(2,1), D(3,2)\n";

        // Run FCFS
        cout << "\n>>> FCFS:\n";
        FCFS_Scheduler fcfs;
        fcfs.schedule(processes);

        // Run SJF
        cout << "\n>>> SJF (non-preemptive):\n";
        SJF_Scheduler sjf;
        sjf.schedule(processes);

        // Run Round Robin with quantum 2
        cout << "\n>>> Round Robin (quantum=2):\n";
        RR_Scheduler rr(2);
        rr.schedule(processes);

        // demonstrate memory
        cout << "\n>>> Memory demo (100 units total):\n";
        memmgr.print_memory_map();
        memmgr.allocate_first_fit(1, 30);
        memmgr.allocate_first_fit(2, 20);
        memmgr.allocate_best_fit(3, 10);
        memmgr.print_memory_map();
        memmgr.deallocate_by_pid(2);
        memmgr.print_memory_map();

        // demo file system
        cout << "\n>>> File system demo:\n";
        fs.ls();
        fs.mkdir("docs");
        fs.touch("readme.txt");
        fs.ls();
        fs.cd("docs");
        fs.pwd();
        fs.touch("notes.txt");
        fs.ls();
        fs.cd("..");
        fs.pwd();
    }
};

/* ============================
   Main
   ============================ */

int main() {
    ios::sync_with_stdio(false);
    // cin.tie(nullptr);

    MiniOSKernelSimulator sim;
    sim.run_shell();
    return 0;
}
