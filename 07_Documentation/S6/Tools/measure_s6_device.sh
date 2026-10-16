#!/system/bin/sh
# Bounded foreground-guarded measurement; USB is only transport, not its clock.
set -eu
pkg=com.indianrailway.s1harness
base=/sdcard/Android/data/$pkg/files/UnrealGame/MyProject
saved=$base/MyProject/Saved
root=${1:-/data/local/tmp/ir-s6-final-measurement}
mode=${2:-all}
mkdir -p "$root"
now() { cut -d. -f1 /proc/uptime | cut -d' ' -f1; }
foreground() { dumpsys window | grep mCurrentFocus | grep -q 'com.indianrailway.s1harness'; }
cleanup() { rm -f "$base/UECommandLine.txt"; }
trap cleanup EXIT
run_phase() {
 phase=$1
 duration=$2
 out=$root/$phase
 mkdir -p "$out/samples" "$out/Saved/S6" "$out/Saved/Logs" "$out/Saved/Profiling/CSV"
 am force-stop "$pkg"
 am start -W -n "$pkg/com.epicgames.unreal.SplashActivity" > "$out/launch.txt"
 sleep 8
 foreground || { echo 'Game not foreground at launch' > "$root/FAILED"; exit 1; }
 start=$(now)
 event=1
 next_sample=0
 next_memory=0
 samples=0
 events=$(wc -l < "$root/$phase-events.txt")
 while :; do
  elapsed=$(($(now)-start))
  [ "$elapsed" -lt "$duration" ] || break
  foreground || { echo "Foreground changed at $phase $elapsed" > "$root/FAILED"; exit 1; }
  if [ "$event" -le "$events" ]; then
   fields=$(sed -n "${event}p" "$root/$phase-events.txt")
   set -- $fields
   if [ "$elapsed" -ge "$1" ]; then
    input tap "$((220+$2*393))" "$((1068+$3*75))"
    printf '{"seconds":%s,"column":%s,"row":%s}\n' "$elapsed" "$2" "$3" >> "$out/touches.jsonl"
    event=$((event+1))
   fi
  fi
  if [ "$elapsed" -ge "$next_sample" ]; then
   candidate=$out/samples/$elapsed.json
   attempts=0
   while :; do
    cp "$saved/S6/telemetry.json" "$candidate"
    if tail -c 1 "$candidate" | grep -q '}'; then break; fi
    attempts=$((attempts+1))
    [ "$attempts" -lt 5 ] || { echo "Incomplete telemetry at $phase $elapsed" > "$root/FAILED"; exit 1; }
    sleep 0.1
   done
   samples=$((samples+1))
   next_sample=$((elapsed+2))
  fi
  if [ "$elapsed" -ge "$next_memory" ]; then
   dumpsys meminfo "$pkg" > "$out/memory-$elapsed.txt"
   dumpsys thermalservice > "$out/thermal-$elapsed.txt"
   echo "$phase $elapsed / $duration seconds; $samples samples" > "$root/status.txt"
   next_memory=$((elapsed+30))
  fi
  sleep 0.2
 done
 cp "$saved/S6/"* "$out/Saved/S6/"
 cp "$saved/Logs/MyProject.log" "$out/Saved/Logs/MyProject.log"
 if [ "$phase" = profile-final ]; then
  latest=$(ls -t "$saved/Profiling/CSV/"*.csv | head -n 1)
  cp "$latest" "$out/Saved/Profiling/CSV/"
 fi
 screencap -p "$out/screen.png"
 printf '{"complete":true,"seconds":%s,"samples":%s,"events":%s,"runner":"bounded Android shell; monotonic uptime"}\n' "$(($(now)-start))" "$samples" "$((event-1))" > "$out/completion.json"
}
if [ "$mode" != sustained ]; then
 printf '%s\n' '-project="../../../MyProject/MyProject.uproject" /Game/IndianRailway/S6/L_RailContinuity -csvCaptureFrames=6000 -csvGpuStats -csvCompression=0 -ExecCmds="r.SetFramePace 30" -log' > "$base/UECommandLine.txt"
 run_phase profile-final 245
 cleanup
 printf '{"restored":true,"previouslyExisted":false}\n' > "$root/profile-final/commandline-restore.json"
fi
run_phase sustained-final 450
echo 'Requested bounded phases completed' > "$root/COMPLETE"
