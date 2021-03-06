#include <Windows.h>
#include <sddl.h>
#include <aclapi.h>
#include <comdef.h>  // for _com_error
#include <iostream>
#include <string>


static void WIN32_CHECK(BOOL res) {
    if (res)
        return;

    _com_error error(GetLastError());
    const wchar_t * w_msg = error.ErrorMessage();
    std::wcerr << L"ERROR: " << w_msg << L"\n";
    abort();
}


/** Tag a folder path as writable by low-integrity processes.
    By default, only %USER PROFILE%\AppData\LocalLow is writable.
    Based on "Designing Applications to Run at a Low Integrity Level" https://msdn.microsoft.com/en-us/library/bb625960.aspx */
static DWORD MakePathLowIntegrity(std::wstring path) {
    ACL * sacl = nullptr; // system access control list
    PSECURITY_DESCRIPTOR SD = nullptr;
    {
        // initialize "low integrity" System Access Control List (SACL)
        // Security Descriptor String interpretation: (based on sddl.h)
        // SACL:(ace_type=Integrity label; ace_flags=; rights=SDDL_NO_WRITE_UP; object_guid=; inherit_object_guid=; account_sid=Low mandatory level)
        WIN32_CHECK(ConvertStringSecurityDescriptorToSecurityDescriptorW(L"S:(ML;;NW;;;LW)", SDDL_REVISION_1, &SD, NULL));
        BOOL sacl_present = FALSE;
        BOOL sacl_defaulted = FALSE;
        WIN32_CHECK(GetSecurityDescriptorSacl(SD, &sacl_present, &sacl, &sacl_defaulted));
    }

    // apply "low integrity" SACL
    DWORD ret = SetNamedSecurityInfoW(const_cast<wchar_t*>(path.data()), SE_FILE_OBJECT, LABEL_SECURITY_INFORMATION, /*owner*/NULL, /*group*/NULL, /*Dacl*/NULL, sacl);
    LocalFree(SD);
    if (ret == ERROR_SUCCESS)
        return ret; // success

    // ERROR_FILE_NOT_FOUND ///< 2
    // ERROR_ACCESS_DENIED  ///< 5
    return ret; // failure
}

int wmain(int argc, wchar_t *argv[])
{
    if (argc != 2) {
        std::wcout << L"Utility to make filesystem paths writable from low-integrity processes.\n";
        std::wcout << L"Usage: MakeLowIntegrity <path>\n";
        return 1;
    }

    std::wstring path = argv[1];
    std::wcout << L"Making path low-integrity: " << path << std::endl;

    DWORD err = MakePathLowIntegrity(path.c_str());
    if (!err) {
        std::wcout << L"Success." << std::endl;
        return 0; // success
    }

    _com_error error(err);
    std::wcerr << L"ERROR: " << error.ErrorMessage() << L" (" << err << L")" << std::endl;
    return 2;
}
