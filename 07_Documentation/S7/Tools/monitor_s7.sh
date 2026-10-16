#!/system/bin/sh
# Observe an already-started Assisted service. Only Depart is automated, after
# logical dwell completion. No route, train position or result is injected.
set -eu
pkg=com.indianrailway.s1harness
saved=/sdcard/Android/data/$pkg/files/UnrealGame/MyProject/MyProject/Saved
out=$1
mode=${2:-assisted}
mkdir -p "$out/samples" "$out/S7"
now() { cut -d. -f1 /proc/uptime | cut -d' ' -f1; }
foreground() { dumpsys window | grep mCurrentFocus | grep -q 'com.indianrailway.s1harness'; }
start=$(now)
next_memory=0
last_station=none
depart_sent=0
power_sent=0
while :; do
 elapsed=$(($(now)-start))
 [ "$elapsed" -lt 600 ] || { echo 'Service monitor timed out' > "$out/FAILED"; exit 1; }
 foreground || { echo 'Game left foreground; no touch sent' > "$out/FAILED"; exit 1; }
 sample=$out/samples/$elapsed.json
 cp "$saved/S7/telemetry.json" "$sample"
 tail -c 1 "$sample" | grep -q '}' || { sleep 1; continue; }
 phase=$(sed -n 's/.*"service":.*"phase":"\([^"]*\)".*/\1/p' "$sample")
 station=$(sed -n 's/.*"service":.*"station":"\([^"]*\)".*/\1/p' "$sample")
 if [ "$station" != "$last_station" ]; then
  screencap -p "$out/station-$station.png"
  last_station=$station
 fi
 if [ "$phase" = Result ]; then
  cp "$saved/S7/"* "$out/S7/"
  dumpsys meminfo "$pkg" > "$out/memory-final.txt"
  screencap -p "$out/result.png"
  printf '{"complete":true,"seconds":%s,"departTouches":%s}\n' "$elapsed" "$depart_sent" > "$out/completion.json"
  exit 0
 fi
 if ! grep -q '"paused":false,"blocked":false' "$sample"; then
  echo 'Service paused or blocked; retained evidence, no automatic resume' > "$out/FAILED"
  exit 1
 fi
 if [ "$mode" = overshoot ] && [ "$power_sent" -eq 0 ] && grep -q '"trafficCleared":true' "$sample"; then
  foreground || exit 1
  input tap 1795 1137
  input tap 1400 1137
  power_sent=1
  printf '{"seconds":%s,"action":"Standard overshoot test: release brake and full power"}\n' "$elapsed" >> "$out/touches.jsonl"
 fi
 if [ "$mode" = assisted ] && [ "$station" = ReadyToDepart ] && [ "$depart_sent" -eq 0 ] && grep -q '"departureAuthorized":false' "$sample"; then
  foreground || exit 1
  input tap 1225 1215
  depart_sent=1
  printf '{"seconds":%s,"action":"Depart","x":1225,"y":1215}\n' "$elapsed" >> "$out/touches.jsonl"
 fi
 if [ "$elapsed" -ge "$next_memory" ]; then
  dumpsys meminfo "$pkg" > "$out/memory-$elapsed.txt"
  dumpsys thermalservice > "$out/thermal-$elapsed.txt"
  printf '%s seconds; %s / %s\n' "$elapsed" "$phase" "$station" > "$out/status.txt"
  next_memory=$((elapsed+30))
 fi
 sleep 1
done
