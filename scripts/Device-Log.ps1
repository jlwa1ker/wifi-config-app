# Confirm the board's current port first (it can shift on reset)
arduino-cli board list

# Then capture to a file (adjust COM11 if board list shows a different port)
Start-Process arduino-cli `
  -ArgumentList 'monitor','-p','COM11','--config','baudrate=115200,dtr=off' `
  -RedirectStandardOutput "C:\Users\walke\git\wifi-config-app\longevity.log" `
  -WindowStyle Hidden

# See the running monitor (note its Id)
Get-Process arduino-cli

# Stop it
Stop-Process -Name arduino-cli
