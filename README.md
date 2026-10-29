# SSH XML Manager

A small Qt Widgets desktop prototype for downloading a configuration file from an SSH host, inspecting selected XML attributes, comparing the downloaded copy with a local replacement, and uploading that replacement.

## Requirements

- Qt 5 Widgets development tools (`qmake`, `moc`, and `uic`) and a C++11 compiler.
- OpenSSH `scp` on `PATH`.
- SSH key or agent authentication and a verified host key for the remote host. Transfers run in batch mode; the application cannot answer password or host-key prompts.

On Debian/Ubuntu, the build dependencies are `build-essential qtbase5-dev openssh-client`. No libssh dependency is needed: transfers use the system `scp` executable.

## Build

```sh
git clone https://github.com/andreystashev/ssh-xml-manager.git
cd ssh-xml-manager
mkdir build
cd build
qmake ../ssh.pro
make -j2
./ssh-xml-manager
```

The project can also be opened in Qt Creator using `ssh.pro`. The build and regression tests have been checked with Qt 5 on Linux.

## Use

The remote connection is currently configured as `root@192.168.1.1:/root/` in `mainwindow.cpp`. Adapt `remoteDirectory` to your device and use an account with suitable permissions. Verify that authentication works from a terminal before using the application.

1. Set the left download folder and right local replacement folder, then press **Update folder** for each. The fields initially point to `~/ssh-xml-manager/downloads` and `~/ssh-xml-manager/files`.
2. Enter a single file name in the bottom field (default: `pi.xml`). All operations use this same name. Supported names begin with a letter, digit, or underscore and contain only letters, digits, underscores, dots, and hyphens.
3. Use the left **Open file** to download and inspect the remote file, or **Download file** to download it without inspecting it. The download folder is created if needed. The right **Open file** inspects the local replacement.
4. Press **Compare** to compare the two local copies byte for byte. Download again to refresh the remote copy before comparing. Missing or unreadable files produce an error.
5. Press **Update files** to upload the file from the right local replacement folder to the remote directory. A successful transfer overwrites the remote file; keep a backup first. The left downloaded copy is not refreshed automatically.

The XML view displays `ClassTag`, `id`, `filename`, `RUS`, `CharGroupId`, and `Value` attributes and reports XML parsing errors. It does not perform schema validation or semantic XML comparison.

## Regression tests

Run these commands from the repository root:

```sh
mkdir -p build/tests
cd build/tests
qmake ../../tests/tests.pro
make -j2
QT_QPA_PLATFORM=offscreen ./tst_mainwindow
```

The tests use a temporary fake `scp` executable and local fixtures. They cover missing files, selected-file comparisons, paths containing spaces, failed transfers, uploading the replacement copy, and rejecting invalid remote names. They do not contact a remote host. The fake executable requires a POSIX shell.

## Prototype limitations

The host, user, and remote directory are configured in source. Transfers are synchronous and can block the interface for up to 30 seconds. Updates have no automatic backup or rollback. Live SSH connectivity and the behavior of a particular remote device require a separate manual check.

<img width="719" alt="SSH XML Manager desktop interface" src="https://github.com/andreystashev/ssh-xml_transfer/assets/70208784/4a60217e-bec0-4566-b482-babfd280d6a4">
