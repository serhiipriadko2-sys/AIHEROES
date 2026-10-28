# AI HEROES — Android readiness audit (2026-10-28)

Status: **BLOCKED / read-only prerequisite check**. This is not an APK test.

## What the connected Windows workstation actually has

| Component | Evidence | Result |
|---|---|---|
| Unreal Engine | UE 5.8.4 installed | PASS for PC development |
| Android Studio | Installed, embedded JBR exists | PASS |
| SDK root | `%LOCALAPPDATA%\Android\Sdk` | PASS |
| Android SDK API 35 | Only `android-36.1` observed; `android-35` missing | BLOCKED |
| Android Build Tools 35 | `build-tools\35.0.0` | PASS |
| Android Platform Tools | `platform-tools` | PASS |
| NDK r27c | No `ndk` folder in Android SDK | BLOCKED |
| Unreal Turnkey SDK validation | `Android INVALID r27c` | BLOCKED |

The existing Java/Android setup is insufficient for a UE 5.8 Android build.
This report does not claim a complete Android compatibility matrix or successful mobile game runtime.

## Safe next gates

1. In Android Studio's SDK Manager, install **Android SDK Platform 35** and the Unreal 5.8-supported **NDK r27c**. Check exact package in the installed UE platform metadata before installing.
2. Set paths in Unreal **Project Settings → Platforms → Android SDK** if automatic SDK discovery does not work. Do not change global JAVA_HOME or ANDROID_HOME without first checking other apps.
3. Run Unreal Turnkey/SDK validation again; require `Android VALID`, not merely SDK folders present.
4. Configure Android input and rendering settings, package a **development** APK, and test on an actual Android device (tap targets, performance, heat, memory, save/load).
5. Keep the default rules-based AI fully offline; a true 1B/3B/8B model adapter needs separate device RAM and latency measurements.

Command to repeat this read-only preflight on the computer:

```powershell
& 'C:\github\AIHEROES\Tools\check_android_readiness.ps1'
```

No SDK downloads, installations or OS environment mutations were performed by this audit.
