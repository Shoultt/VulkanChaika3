#include <windows.h>

int CALLBACK WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR     lpCmdLine,
    int       nCmdShow
)
{
    const HDESK hOld = GetThreadDesktop(GetCurrentThreadId());

    const HDESK hNew = CreateDesktop(
        TEXT("������"),
        NULL,
        NULL,
        0,
        DESKTOP_SWITCHDESKTOP
            | DESKTOP_WRITEOBJECTS
            | DESKTOP_READOBJECTS
            | DESKTOP_CREATEWINDOW,
        NULL
    );

    while (true)
    {
        SetThreadDesktop(hNew);
        SwitchDesktop(hNew);

        MessageBox(NULL, TEXT("�������������"), TEXT("������ �� ��������"), MB_ICONWARNING);

        SwitchDesktop(hOld);
        CloseDesktop(hNew);
    }
    return 0;
}