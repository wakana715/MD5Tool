# .DESCRIPTION
# MD5ToolのGUIスクリプト
# MD5Tool.ps1 prm status progress
# .PARAMETER	Arg[0]		[Out]		パラメータファイル、GUIの入力値を出力する
# .PARAMETER	Arg[1]		[In,Out]	ステータスファイル、状態を入出力する
# .PARAMETER	Arg[2]		[In]		プログレスファイル、進捗を取得する
if ($Args.Count -lt 3)
{
    exit
}
Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing
$parameterFile  = $Args[0]
$statusFile     = $Args[1]
$progressFile   = $Args[2]
$form			= New-Object System.Windows.Forms.Form
$comboBoxDir	= New-Object System.Windows.Forms.ComboBox
$buttonOpen		= New-Object System.Windows.Forms.Button
$labelInfo		= New-Object System.Windows.Forms.Label
$buttonRun		= New-Object System.Windows.Forms.Button
$buttonStop		= New-Object System.Windows.Forms.Button
$timer			= New-Object System.Windows.Forms.Timer
$titleDirectory	= "Directory"

# .DESCRIPTION
# Directoryのオープンボタンイベント関数。
# フォルダ選択ダイアログでフォルダ名を入力する。
# 入力したフォルダ名を引数指定のコントロールへ設定する。
# .PARAMETER	senderFrom	未使用
# .PARAMETER	e			未使用
function buttonOpen_Click()
{
    param
    (
        [System.Object]$senderFrom,
        [System.EventArgs]$e
    )
    $buttonOpen.Enabled = $False
    $form.Refresh()
    $dialog=New-Object System.Windows.Forms.FolderBrowserDialog
    $dialog.Description="Select " + $titleDirectory
    if ($dialog.ShowDialog() -eq [System.Windows.Forms.DialogResult]::OK)
    {
        $comboBoxDir.Text = $dialog.SelectedPath
    }
    $buttonOpen.Enabled = $True
    $form.Refresh()
}

# .DESCRIPTION
# Stopボタンのイベント関数。
# .PARAMETER	senderFrom	未使用
# .PARAMETER	e			未使用
function buttonStop_Click()
{
    param
    (
        [System.Object]$senderFrom,
        [System.EventArgs]$e
    )
    Set-Content -Path $statusFile -Value "0003" -Encoding ascii # 3:中断要求
}

# .DESCRIPTION
# Runボタンのイベント関数。
# .PARAMETER	senderFrom	未使用
# .PARAMETER	e			未使用
function buttonRun_Click()
{
    param
    (
        [System.Object]$senderFrom,
        [System.EventArgs]$e
    )
    $path = $comboBoxDir.Text
    if ($path -eq "" -Or (Test-Path $path) -eq $False)
    {
        [System.Windows.Forms.MessageBox]::Show($path + " is not found", "Error", "OK", "Error")
        return
    }
    $res = [System.Windows.Forms.MessageBox]::Show("Do you want to run it?", "Information", 1, "Information") # 1:[OK][Cancel]
    if ($res -eq "OK")
    {
        $comboBoxDir.Enabled = $False
        $buttonOpen.Enabled = $False
        $buttonStop.Enabled = $True
        $buttonRun.Enabled = $False
        $labelInfo.Text = ""
        $form.Refresh()
        $value = "dir=" + $path
        Set-Content -Path $parameterFile -Value "K=V"  -Encoding unicode # DUMMY
        Add-Content -Path $parameterFile -Value $value -Encoding unicode
        Set-Content -Path $statusFile    -Value "0001" -Encoding ascii # 1:実行要求
        $timer.Enabled = $True
    }
}

# .DESCRIPTION
# タイマイベント関数。
# .PARAMETER	senderFrom	未使用
# .PARAMETER	e			未使用
function timer_Tick()
{
    param
    (
        [System.Object]$senderFrom,
        [System.EventArgs]$e
    )
    $strStatus = Get-Content -Path $statusFile -Encoding ascii
    $status = [Convert]::ToInt64($strStatus, 16)
    if ($status -eq 0)		# 0:NOP
    {
        $timer.Enabled = $False
        $comboBoxDir.Enabled = $True
        $buttonOpen.Enabled = $True
        $buttonStop.Enabled = $False
        $buttonRun.Enabled = $True
        $labelInfo.Text = "Finished"
        $form.Refresh()
    }
    if ($status -eq 2)		# 2:実行中
    {
        $path = Get-Content -Path $progressFile -Encoding unicode
        $labelInfo.Text = $path
        $form.Refresh()
    }
    if ($status -eq 3)		# 3:中断要求
    {
        $timer.Enabled = $False
        $comboBoxDir.Enabled = $True
        $buttonOpen.Enabled = $True
        $buttonStop.Enabled = $False
        $buttonRun.Enabled = $True
        $labelInfo.Text = "Stop"
        $form.Refresh()
        Set-Content -Path $statusFile -Value "0000" -Encoding ascii # 0:NOP
    }
}

Set-Content -Path $statusFile -Value "0000" -Encoding ascii # 0:NOP
$Font = New-Object System.Drawing.Font("ＭＳ ゴシック",12)

$form.Text = "MD5Tool"
$form.Size = New-Object System.Drawing.Size(520,200)
$form.MinimumSize = New-Object System.Drawing.Size(320,200)
$form.StartPosition = "CenterScreen"
$form.font = $Font

$comboBoxDir.Location = New-Object System.Drawing.Point(100,10)
$comboBoxDir.size = New-Object System.Drawing.Size(350,30)
$comboBoxDir.Anchor = 1 + 4 + 8 # 1:Top + 4:Left + 8:Right
$comboBoxDir.DropDownStyle = "DropDown"
$comboBoxDir.FlatStyle = "standard"
$comboBoxDir.font = $Font
$comboBoxDir.TabIndex = 0

$comboBoxHeight = $comboBoxDir.Size.Height

$labelDirectory = New-Object System.Windows.Forms.Label
$labelDirectory.Location = New-Object System.Drawing.Point(10,10)
$labelDirectory.Size = New-Object System.Drawing.Size(90,$comboBoxHeight)
$labelDirectory.Text = $titleDirectory
$labelDirectory.TextAlign = 16 # 16:MiddleLeft
$form.Controls.Add($labelDirectory)

$buttonOpen.Location = New-Object System.Drawing.Point(450,10)
$buttonOpen.Size = New-Object System.Drawing.Size(40,$comboBoxHeight)
$buttonOpen.Anchor = 1 + 8 # 1:Top + 8:Right
$buttonOpen.Text = "..."
$buttonOpen.Add_Click({param($s,$e) buttonOpen_Click $s $e})
$form.Controls.Add($buttonOpen)

$labelInfo.BorderStyle = 1 # 1:FixedSingle
$labelInfo.Location = New-Object System.Drawing.Point(10,40)
$labelInfo.Size = New-Object System.Drawing.Size(480,60)
$labelInfo.Anchor = 1 + 2 + 4 + 8 # 1:Top + 2:Bottom + 4:Left + 8:Right
$labelInfo.Text = ""
$form.Controls.Add($labelInfo)

$buttonStop.Location = New-Object System.Drawing.Point(280,120)
$buttonStop.Size = New-Object System.Drawing.Size(100,30)
$buttonStop.Anchor = 2 + 8 # 2:Bottom + 8:Right
$buttonStop.Text = "Stop"
$buttonStop.Enabled = $False
$buttonStop.Add_Click({param($s,$e) buttonStop_Click $s $e})
$form.Controls.Add($buttonStop)

$buttonRun.Location = New-Object System.Drawing.Point(390,120)
$buttonRun.Size = New-Object System.Drawing.Size(100,30)
$buttonRun.Anchor = 2 + 8 # 2:Bottom + 8:Right
$buttonRun.Text = "Run"
$buttonRun.Add_Click({param($s,$e) buttonRun_Click $s $e})
$form.Controls.Add($buttonRun)

$timer.Enabled = $False
$timer.Interval = 500
$timer.Add_Tick({param($s,$e) timer_Tick $s $e})

$xmlPath = $PSScriptRoot + "\MD5Tool.xml"
# xml 読込
# TIPS:StreamReader を使用しないと "他のプロセスが使用中" になる
$sr = [System.IO.StreamReader]::new([System.IO.FileStream]::new($xmlPath,
      [System.IO.FileMode]::Open, [System.IO.FileAccess]::Read,
      [System.IO.FileShare]::ReadWrite + [System.IO.FileShare]::Delete))
$xmlTextReader = New-Object System.Xml.XmlTextReader($sr)
$isDirectory = $false
$Directory = ""
while ($xmlTextReader.Read())
{
    if ($xmlTextReader.NodeType.Equals([System.Xml.XmlNodeType]::Element))
    {
        $isDirectory = $false
        if ($xmlTextReader.Name.Equals($titleDirectory))
        {
            $isDirectory = $true
        }
    }
    elseif ($xmlTextReader.NodeType.Equals([System.Xml.XmlNodeType]::Text))
    {
        if ($isDirectory)
        {
            $Directory = $xmlTextReader.Value
        }
    }
}
$xmlTextReader.Close()
$xmlTextReader.Dispose()
$sr.Close()
$sr.Dispose()

# コンボボックスへ項目を設定
$i = 0
foreach ($path in $Directory.Split("`n"))
{
    $path = $path.Trim()
    if ($path.Length -gt 0)
    {
        if ($i -eq 0)
        {
            $comboBoxDir.Text = $path
        }
        [void]$comboBoxDir.Items.Add($path)
        $i++
    }
}
$form.Controls.Add($comboBoxDir)

$form.ShowDialog()

# コンボボックスの項目を xml に保存
$crlf = [char]13 + [char]10
$Directory = $crlf
$text = $comboBoxDir.Text.Trim()
if ($text.Length -gt 0)
{
    $Directory += $text + $crlf
}
foreach ($item in $comboBoxDir.Items)
{
    if ($item -ne $text)
    {
        $Directory += $item + $crlf
    }
}

# TIPS:StreamWriter を使用しないと "他のプロセスが使用中" になる
$sw = New-Object System.IO.StreamWriter($xmlPath, $false, [System.Text.Encoding]::UTF8)
$xmlTextWriter = New-Object System.Xml.XmlTextWriter($sw)
$xmlTextWriter.WriteStartDocument()
$xmlTextWriter.WriteWhitespace($crlf)
$xmlTextWriter.WriteStartElement("Settings")
$xmlTextWriter.WriteString($crlf)
$xmlTextWriter.WriteElementString($titleDirectory, $Directory)
$xmlTextWriter.WriteWhitespace($crlf)
$xmlTextWriter.WriteEndElement()
$xmlTextWriter.WriteWhitespace($crlf)
$xmlTextWriter.WriteEndDocument()
$xmlTextWriter.Close()
$xmlTextWriter.Dispose()
$sw.Close()
$sw.Dispose()
