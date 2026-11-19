param([string]$Target = 'help')
# Windows PowerShell 5.1+, native MinGW-w64 GCC; no sh/awk/MSYS dependency.
Set-StrictMode -Version 2
$ErrorActionPreference = 'Stop'
Set-Location -LiteralPath (Split-Path -Parent $PSScriptRoot)
if ($Target -eq 'help') {
    Write-Output @'
Exercise 6 native C17
make help/check/examples/all/test-a/test-b/test/example-register/example-register-file/example-memory/test-student/clean
check: native C17 compile/link/run; examples: supplied parts only
all: supplied parts and build checks (unfinished tasks allowed)
test-a: Register/Decoder5/MUX32 -> RegisterFile; test-b: independent IM/DM
CC: one executable path; default gcc. CFLAGS: whitespace-separated flags, default -O0 -g
tools/run.ps1 can also be called directly to observe the runner exit code.
'@
    exit 0
}
$targets = @('check','examples','all','test-a','test-b','test','example-register','example-register-file','example-memory','test-student','clean')
if ($targets -notcontains $Target) { Write-Output '[ENV] unknown target'; exit 2 }
function Assert-RealDirectory([string]$Path) {
    if (Test-Path -LiteralPath $Path) {
        $item = Get-Item -LiteralPath $Path -Force
        if (-not $item.PSIsContainer -or ($item.Attributes -band [IO.FileAttributes]::ReparsePoint)) {
            throw "[ENV] refusing non-directory or reparse point: $Path"
        }
    }
}
try { Assert-RealDirectory 'build' } catch { Write-Output $_; exit 2 }
if (-not (Test-Path -LiteralPath 'build')) { [void](New-Item -ItemType Directory -Path 'build') }
if ($Target -eq 'clean') {
    $items = @(Get-ChildItem -LiteralPath 'build' -Force | Where-Object { $_.Name -like 'run-*' })
    try {
        foreach ($item in $items) {
            Assert-RealDirectory $item.FullName
            $links = @(Get-ChildItem -LiteralPath $item.FullName -Force -Recurse |
                Where-Object { $_.Attributes -band [IO.FileAttributes]::ReparsePoint })
            if ($links.Count) { throw '[ENV] refusing reparse point inside build' }
        }
        foreach ($item in $items) { Remove-Item -LiteralPath $item.FullName -Force -Recurse }
    } catch { Write-Output $_; exit 2 }
    Write-Output '[PASS] clean (student files and build/.gitkeep retained)'
    exit 0
}
$run = 'build/run-' + [Guid]::NewGuid().ToString('N')
[void](New-Item -ItemType Directory -Path $run)
Write-Output "LOGDIR=$run"
function Read-Limit([string]$Name, [int]$Default, [int]$Minimum) {
    $value = [Environment]::GetEnvironmentVariable($Name)
    if ([string]::IsNullOrEmpty($value)) { return $Default }
    $number = 0
    if (-not [int]::TryParse($value,[ref]$number) -or $number -lt $Minimum) {
        throw "[ENV] invalid $Name"
    }
    return $number
}
try {
    $buildSeconds = Read-Limit 'EX06_BUILD_TIMEOUT' 120 1
    $seconds = Read-Limit 'EX06_RUN_TIMEOUT' 20 1
    $limit = Read-Limit 'EX06_OUTPUT_LIMIT' 1048576 1024
    $compiler = $env:CC
    if ([string]::IsNullOrWhiteSpace($compiler)) { $compiler = 'gcc' }
    $resolved = Get-Command -Name $compiler -CommandType Application -ErrorAction Stop
    $ccPath = $resolved.Source
    $env:PATH = (Split-Path -Parent $ccPath) + ';' + $env:PATH
} catch { Write-Output "[ENV] CC/limits invalid; no fallback: $_"; exit 2 }
$cflags = if ([string]::IsNullOrWhiteSpace($env:CFLAGS)) { @('-O0','-g') } else { @($env:CFLAGS -split '\s+' | Where-Object { $_ }) }
$ldflags = if ([string]::IsNullOrWhiteSpace($env:LDFLAGS)) { @() } else { @($env:LDFLAGS -split '\s+' | Where-Object { $_ }) }
@("CC=$ccPath","CFLAGS=$($cflags -join ' ')","LDFLAGS=$($ldflags -join ' ')") |
    Set-Content -LiteralPath "$run/config.txt" -Encoding UTF8
function Quote-Argument([string]$Value) {
    return '"' + [regex]::Replace([regex]::Replace($Value,'(\\*)"','$1$1\"'),'(\\+)$','$1$1') + '"'
}
function Stop-OwnedTree([int]$ParentId) {
    $children = @(Get-CimInstance Win32_Process -Filter "ParentProcessId=$ParentId" -ErrorAction SilentlyContinue)
    foreach ($child in $children) { Stop-OwnedTree ([int]$child.ProcessId) }
    Stop-Process -Id $ParentId -Force -ErrorAction SilentlyContinue
}
$guard = "$run/guard.exe"
$bootstrapArgs = @('-std=c17','-Wall','-Wextra','-Wpedantic','tools/guard.c','-lshell32','-o',$guard)
$bootstrap = $null
try {
    $argumentLine = ($bootstrapArgs | ForEach-Object { Quote-Argument $_ }) -join ' '
    $bootstrap = Start-Process -FilePath $ccPath -ArgumentList $argumentLine -NoNewWindow -PassThru -RedirectStandardOutput "$run/bootstrap.stdout" -RedirectStandardError "$run/bootstrap.stderr"
    "pid=$($bootstrap.Id)" | Set-Content "$run/bootstrap.meta"
    if (-not $bootstrap.WaitForExit($buildSeconds * 1000)) {
        Stop-OwnedTree $bootstrap.Id
        $bootstrap.WaitForExit()
        throw '[ENV] compiler bootstrap timeout'
    }
    $bootstrap.WaitForExit()
    "raw_exit=$($bootstrap.ExitCode)" | Add-Content "$run/bootstrap.meta"
    if ($bootstrap.ExitCode -ne 0) { throw '[ENV] cannot build native monitor; check compiler' }
} catch {
    if (Test-Path "$run/bootstrap.stderr") { Get-Content "$run/bootstrap.stderr" }
    Write-Output $_
    exit 2
} finally {
    if ($null -ne $bootstrap) {
        if (-not $bootstrap.HasExited) { Stop-OwnedTree $bootstrap.Id; $bootstrap.WaitForExit() }
        $bootstrap.Dispose()
    }
}
$script:serial = 0
$script:childRC = 0
function Invoke-Monitored([string]$Label,[string]$Completion,[int]$Duration,[string[]]$Command) {
    $script:serial++
    $prefix = '{0}/{1:D3}-{2}' -f $run,$script:serial,$Label
    try {
        & $guard run "$Duration" "$limit" $prefix $Completion @Command
        $script:childRC = $LASTEXITCODE
    } catch { Write-Output "[RUNTIME] cannot execute native monitor: $_"; $script:childRC = 7 }
    if (Test-Path "$prefix.stdout") { [Console]::Write([IO.File]::ReadAllText((Join-Path (Get-Location) "$prefix.stdout"))) }
    if (Test-Path "$prefix.stderr") { [Console]::Error.Write([IO.File]::ReadAllText((Join-Path (Get-Location) "$prefix.stderr"))) }
}
function Build-Program([string]$Label,[string]$Output,[string[]]$Sources) {
    $command = @($ccPath,'-std=c17','-Wall','-Wextra','-Wpedantic') + $cflags +
               @('-Iinclude','-Itests') + $Sources + $ldflags + @('-o',$Output)
    Invoke-Monitored "build-$Label" '-' $buildSeconds $command
    if ($script:childRC -ne 0) { Write-Output "[BUILD] $Label compilation/link failed"; $script:childRC = 4 }
}
Invoke-Monitored 'compiler-version' '-' $buildSeconds @($ccPath,'--version') | Out-File "$run/compiler.txt" -Encoding UTF8
if ($script:childRC -ne 0) { Write-Output '[ENV] compiler version query failed'; exit 2 }
Invoke-Monitored 'compiler-target' '-' $buildSeconds @($ccPath,'-dumpmachine') | Out-File "$run/target.txt" -Encoding UTF8
if ($script:childRC -ne 0) { Write-Output '[ENV] compiler target query failed'; exit 2 }
Build-Program 'probe' "$run/probe.exe" @('tools/probe.c')
if ($script:childRC -ne 0) { Write-Output '[ENV] native C17 probe did not build'; exit 2 }
Invoke-Monitored 'check' 'check' $seconds @("$run/probe.exe")
if ($script:childRC -ne 0) { Write-Output '[ENV] generated C17 executable did not run correctly'; exit 2 }
if ($Target -eq 'check') { exit 0 }
$sourcePreference = $ErrorActionPreference
$ErrorActionPreference = 'Continue'
& $guard source 1> "$run/source.stdout" 2> "$run/source.stderr"
$sourceRC = $LASTEXITCODE
$ErrorActionPreference = $sourcePreference
Get-Content "$run/source.stdout"
Get-Content "$run/source.stderr"
if ($sourceRC -ne 0) { exit 8 }
$common = @('src/word.c','src/register_support.c','src/memory_support.c','tasks/register_task.c','tasks/memory_task.c','tests/test_support.c')
$testSources = $common + @('tests/test_parts.c','tests/test_registers.c','tests/test_memory.c','tests/test_errors.c','tests/student_cases.c','tests/test_main.c')
Build-Program 'tests' "$run/tests.exe" $testSources
if ($script:childRC -ne 0) { exit 4 }
$drivers = @{'example-register'='examples/observe_register.c'; 'example-register-file'='examples/observe_register_file.c'; 'example-memory'='examples/observe_memory.c'}
if ($Target -in @('all','examples')) {
    Build-Program 'example-support' "$run/example-support.exe" ($common + @('tests/test_parts.c','examples/support_demo.c'))
    if ($script:childRC -ne 0) { exit 4 }
    if ($Target -eq 'all') {
        foreach ($name in @('example-register','example-register-file','example-memory')) {
            Build-Program $name "$run/$name.exe" ($common + @($drivers[$name]))
            if ($script:childRC -ne 0) { exit 4 }
        }
    }
    Invoke-Monitored 'example-support' 'example-support' $seconds @("$run/example-support.exe")
    if ($script:childRC -ne 0) { exit $script:childRC }
    Write-Output '[PASS] supplied helpers; observation drivers require task implementations.'
    exit 0
}
if ($drivers.ContainsKey($Target)) {
    Build-Program $Target "$run/$Target.exe" ($common + @($drivers[$Target]))
    if ($script:childRC -ne 0) { exit 4 }
    Invoke-Monitored $Target $Target $seconds @("$run/$Target.exe")
    exit $script:childRC
}
function Run-Group([string]$Name) { Invoke-Monitored $Name $Name $seconds @("$run/tests.exe",$Name) }
function Run-Errors([string]$Unit) {
    $script:errorRC = 0
    [long]$base = if ($Unit -in @('im','setter','index-im')) { 4194304 } else { 268500992 }
    $component = if ($base -eq 4194304) { 'InstMemory' } else { 'DataMemory' }
    for ($k = 0; $k -lt 6; $k++) {
        [long]$address = 0; $id = ''
        switch ($k) {
            0 { $address = $base-4; $id = 'ADDR_BELOW' }
            4 { $address = $base+4096; $id = 'ADDR_ABOVE' }
            5 { $address = 4294967295; $id = 'ADDR_ALIGN' }
            default { $address = $base+$k; $id = 'ADDR_ALIGN' }
        }
        $raw = 10
        if ($Unit -in @('control','dm-both')) { $raw = 11; $id = 'CTRL_BOTH' }
        $completion = 'error/{0}/{1}/{2}/{3:x8}/{4:x8}' -f $raw,$id,$component,$address,$base
        Invoke-Monitored "E-$Unit-$k" $completion $seconds @("$run/tests.exe",'error',$Unit,"$k")
        if ($script:childRC -ne 0) { $script:errorRC = 9 }
        if ($Unit -eq 'control') { break }
    }
}
$status = 0
Run-Group 'parts'; $partsRC = $script:childRC
foreach ($unit in @('index-im','index-dm','setter','control')) { Run-Errors $unit; if ($script:errorRC -ne 0) { $partsRC = 9 } }
if ($partsRC -ne 0) { Write-Output '[DEPENDENCY] supplied helpers failed; A/B skipped, cases=0'; exit 9 }
if ($Target -in @('test-a','test','test-student')) {
    Run-Group 'register'; $registerRC = $script:childRC
    Run-Group 'decoder'; $decoderRC = $script:childRC
    Run-Group 'mux'; $muxRC = $script:childRC
    if ($registerRC -eq 0 -and $decoderRC -eq 0 -and $muxRC -eq 0) { Run-Group 'rf'; $rfRC = $script:childRC }
    else { $rfRC = 9; Write-Output '[DEPENDENCY] rf skipped: Register/Decoder5/MUX32 failed; cases=0' }
    if ($registerRC -ne 0 -or $decoderRC -ne 0 -or $muxRC -ne 0 -or $rfRC -ne 0) { $status = 9 }
    Write-Output "SUMMARY A register=$registerRC decoder=$decoderRC mux=$muxRC rf=$rfRC"
}
if ($Target -in @('test-b','test','test-student')) {
    Run-Group 'im'; $imRC = $script:childRC
    if ($imRC -eq 0) { Run-Errors 'im'; if ($script:errorRC -ne 0) { $imRC = 9 } }
    else { Write-Output '[DEPENDENCY] im errors skipped: normal im failed; cases=0' }
    Run-Group 'dm'; $dmRC = $script:childRC
    if ($dmRC -eq 0) {
        foreach ($unit in @('dm-read','dm-write','dm-both')) { Run-Errors $unit; if ($script:errorRC -ne 0) { $dmRC = 9 } }
    } else { Write-Output '[DEPENDENCY] dm errors skipped: normal dm failed; cases=0' }
    if ($imRC -ne 0 -or $dmRC -ne 0) { $status = 9 }
    Write-Output "SUMMARY B im=$imRC dm=$dmRC (independent of A)"
}
if ($Target -eq 'test-student') {
    if ($rfRC -eq 0) { Run-Group 'student-a'; if ($script:childRC -ne 0) { $status = 9 } }
    else { Write-Output '[DEPENDENCY] student-a skipped; cases=0' }
    if ($imRC -eq 0 -and $dmRC -eq 0) { Run-Group 'student-b'; if ($script:childRC -ne 0) { $status = 9 } }
    else { Write-Output '[DEPENDENCY] student-b skipped; cases=0' }
}
if ($status -eq 0) { Write-Output "[PASS] $Target completed" }
exit $status
