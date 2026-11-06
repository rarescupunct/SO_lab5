#include<stdio.h>
#include<iostream>
#include<string>
#include<vector>

#ifdef _WIN32
#include<windows.h>
#else
#include<sys/wait.h>
#include<unistd.h>
#endif

#define PROCESSES 10
#define SIZE 1000

static bool isPrime(int n){
    if(n < 2) return false;
    for(int d = 2; d * d <= n; ++d)
        if(n%d == 0) return false;
    return true;
}

#ifdef _WIN32

static int childMain(){
    int lo, hi;
    std::cin >> lo >> hi;
    for(int n = lo; n <= hi; ++n)
        if(isPrime(n)) std::cout << n << ' ';
    std::cout << '\n';
    return 0;
}

static int parentMain(){
    char exePath[MAX_PATH];
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);

    std::vector<HANDLE> readH(PROCESSES), procH(PROCESSES);

    for(int i = 0; i < PROCESSES; ++i){
        SECURITY_ATTRIBUTES sa{
            sizeof(sa),
            nullptr,
            TRUE
        };
        HANDLE inR, inW, outR, outW;
        if(!CreatePipe(&inR, &inW, &sa, 0) || !CreatePipe(&outR, &outW, &sa, 0)){
            std::cerr << "CreatePipe a esuat\n";
            return 1;
        }

        SetHandleInformation(inW, HANDLE_FLAG_INHERIT, 0);
        SetHandleInformation(outR, HANDLE_FLAG_INHERIT, 0);

        STARTUPINFOA si{};
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESTDHANDLES;
        si.hStdInput = inR;
        si.hStdOutput = outW;
        si.hStdError = GetStdHandle(STD_ERROR_HANDLE);

        PROCESS_INFORMATION pi{};
        std::string cmd = std::string("\"") + exePath + "\" child";
        if(!CreateProcessA(nullptr, &cmd[0], nullptr, nullptr, TRUE, 0,
                            nullptr, nullptr, &si, &pi)){

            std::cerr << "CreateProcess a esuat: " << GetLastError() << "\n";
            return 1;
        }
        CloseHandle(pi.hThread);
        CloseHandle(inR);
        CloseHandle(outW);

        std::string range = std::to_string(i * SIZE + 1) + " " + 
                            std::to_string((i + 1) * SIZE) + "\n";

        DWORD written;
        WriteFile(inW, range.c_str(), (DWORD)range.size(), &written, nullptr);
        CloseHandle(inW);

        readH[i]=outR;
        procH[i]=pi.hProcess;
    }

    int total = 0;
    for(int i = 0; i < PROCESSES; ++i){
        std::printf("----- Process %d, interval %d-%d ---\n", i, i * SIZE + 1, (i + 1) * SIZE);
        std::string data;
        char buf[4096];
        DWORD got;
        while(ReadFile(readH[i], buf, sizeof(buf), &got, nullptr) && got > 0)
            data.append(buf, got);

            int cur = -1;
            for(char c: data){
                if(c >= '0' && c <='9'){
                    cur = (cur < 0 ? 0 : cur) * 10 + (c-'0');
                } else if(cur >= 0){
                    std::printf("%d ", cur);
                    ++total;
                    cur = -1;
                }
            }

            if(cur >= 0){ std::printf("%d ", cur); ++total;}

            CloseHandle(readH[i]);
            WaitForSingleObject(procH[i], INFINITE);
            CloseHandle(procH[i]);
    }

    std::printf("\nTotal numere prime pana la %d: %d\n", PROCESSES * SIZE, total);
    return 0;
}

int main(int argc, char* argv[]){
    if(argc > 1 && std::string(argv[1]) == "child") return childMain();
    return parentMain();
}

#else

#endif