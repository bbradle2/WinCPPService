// WinCPPService.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <tchar.h>
#include <windows.h>
#include <winsvc.h>
#include <iostream>
#include <process.h>
#include <conio.h>



SERVICE_STATUS_HANDLE g_ServiceStatusHandle;
HANDLE g_StopEvent = NULL;
DWORD g_CurrentState = 0;
bool g_SystemShutdown = false;
bool isInteractive = false;

void WINAPI ServiceMain(DWORD argc, LPTSTR* argv);
void ReportStatus(DWORD state);
void ReportProgressStatus(DWORD state, DWORD checkPoint, DWORD waitHint);
DWORD WINAPI HandlerEx(DWORD control, DWORD eventType, void* eventData, void* context);
BOOL IsUserInteractive();
DWORD WINAPI Worker(LPVOID);

int main(int argc, char** argv)
{
    isInteractive = IsUserInteractive();
    if (!isInteractive)
    {

        SERVICE_TABLE_ENTRY serviceTable[] = {
           { (WCHAR*)L"SERVICE NAME", &ServiceMain },
           { NULL, NULL }
        };

        if (StartServiceCtrlDispatcher(serviceTable))
            return 0;
        else if (GetLastError() == ERROR_FAILED_SERVICE_CONTROLLER_CONNECT)
            return -1; // Program not started as a service.
        else
            return -2; // Other error.
    }
    else
    {
        
        HANDLE l_RunThread = CreateThread(NULL, 0, Worker, NULL, 0, NULL);
        g_StopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

        std::cout << "Press 'q' or 'Q' to Exit.." << std::endl;
        while (true) 
        {

            int ch = toupper(_getch());
            if (ch == 'Q') break;
        }

        if (g_StopEvent)
            SetEvent(g_StopEvent);

        if (l_RunThread) {
            WaitForSingleObject(l_RunThread, INFINITE);
            CloseHandle(l_RunThread);
        }
        if (g_StopEvent)
            CloseHandle(g_StopEvent);

        return 0;
    }
}

DWORD WINAPI Worker(LPVOID lpParam)
{
    UNREFERENCED_PARAMETER(lpParam);
    //  Periodically check if the service has been requested to stop
    while (WaitForSingleObject(g_StopEvent, 3000) != WAIT_OBJECT_0)
    {
        //Beep(13500, 3000);
        std::cout << "Worker running..." << std::endl;
    }

    std::cout << "Worker stopped." << std::endl;

    return ERROR_SUCCESS;
}

BOOL IsUserInteractive()
{
    BOOL bIsUserInteractive = TRUE;

    HWINSTA hWinStation = GetProcessWindowStation();
    if (hWinStation != NULL)
    {
        USEROBJECTFLAGS uof = { 0 };
        if (GetUserObjectInformation(hWinStation, UOI_FLAGS, &uof, sizeof(USEROBJECTFLAGS), NULL) && ((uof.dwFlags & WSF_VISIBLE) == 0))
        {
            bIsUserInteractive = FALSE;
        }
    }
    return bIsUserInteractive;
}

void WINAPI ServiceMain(DWORD argc, LPTSTR* argv)
{
    // Must be called at start.
    g_ServiceStatusHandle = RegisterServiceCtrlHandlerEx(_T("SERVICE NAME"), &HandlerEx, NULL);

    // Startup code.
    ReportStatus(SERVICE_START_PENDING);
    g_StopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    /* Here initialize service...
    Load configuration, acquire resources etc. */
    ReportStatus(SERVICE_RUNNING);

    /* Main service code
    Loop, do some work, block if nothing to do,
    wait or poll for g_StopEvent... 
    */
    if (g_StopEvent) 
    {
        HANDLE l_RunThread = CreateThread(NULL, 0, Worker, NULL, 0, NULL);
        if (l_RunThread)
        {
            WaitForSingleObject(l_RunThread, INFINITE);
            CloseHandle(l_RunThread);
        }

        //while (WaitForSingleObject(g_StopEvent, INFINITE) != WAIT_OBJECT_0)
        //{
        //    // This sample service does "BEEP!" every 3 seconds.
        //    //Beep(1000, 100);
        //}
    }

    ReportStatus(SERVICE_STOP_PENDING);
    /* Here finalize service...
    Save all unsaved data etc., but do it quickly.
    If g_SystemShutdown, you can skip freeing memory etc. */
    if(g_StopEvent)
        CloseHandle(g_StopEvent);

    ReportStatus(SERVICE_STOPPED);
}



void ReportStatus(DWORD state)
{
    g_CurrentState = state;
    SERVICE_STATUS serviceStatus = {
        SERVICE_WIN32_OWN_PROCESS,
        g_CurrentState,
        state == SERVICE_START_PENDING ? 0 : DWORD(SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN),
        NO_ERROR,
        0,
        0,
        0,
    };
    SetServiceStatus(g_ServiceStatusHandle, &serviceStatus);
}

void ReportProgressStatus(DWORD state, DWORD checkPoint, DWORD waitHint)
{
    g_CurrentState = state;
    SERVICE_STATUS serviceStatus = {
        SERVICE_WIN32_OWN_PROCESS,
        g_CurrentState,
        state == SERVICE_START_PENDING ? 0 : DWORD(SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN),
        NO_ERROR,
        0,
        checkPoint,
        waitHint,
    };
    SetServiceStatus(g_ServiceStatusHandle, &serviceStatus);
}

// Handler for service control events.
DWORD WINAPI HandlerEx(DWORD control, DWORD eventType, void* eventData, void* context)
{
    switch (control)
    {
        // Entrie system is shutting down.
    case SERVICE_CONTROL_SHUTDOWN:
        g_SystemShutdown = true;
        // continue...
    // Service is being stopped.
    case SERVICE_CONTROL_STOP:
        ReportStatus(SERVICE_STOP_PENDING);
        SetEvent(g_StopEvent);
        break;
        // Ignoring all other events, but we must always report service status.
    default:
        ReportStatus(g_CurrentState);
        break;
    }
    return NO_ERROR;
}
// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
