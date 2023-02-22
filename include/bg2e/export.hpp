#ifndef BG2E_EXPORT_HPP
#define BG2E_EXPORT_HPP

#ifdef _WIN32

#pragma warning( disable: 4251 )
#pragma warning( disable: 4275 )

#ifdef _WINDLL

#define BG2E_EXPORT __declspec( dllexport )

#else	// not _WIN32

#define BG2E_EXPORT __declspec( dllimport )

#endif	// _WINDLL

#else	// not _WIN32

#define BG2E_EXPORT

#endif	// _WIN32

#endif

