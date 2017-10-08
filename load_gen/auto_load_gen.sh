time_out="5m"
for i in $(seq 10 5 400);
do
  #Uploads
  echo "starting file_server"
  timeout $time_out taskset -c 2 ./file_server  6223 &
  sleep 2

  echo "starting proxy_server"
  timeout $time_out taskset -c 1 ./proxy_server 10.129.23.200 6223 3423 &
  sleep 2
  echo "starting client"
  timeout $time_out taskset -c 3,4,5,6 ./a.out 10.129.23.200 3423 $i 1 &


  cat response_logs/* >  response_logs/total.log
  echo -n $i"," >> logs/up/response_time.csv
  awk '{ total += $1; count++ } END { print total/count }' response_logs/total.log >> logs/up/response_time.csv
  mv response_logs/total.log logs/up/response_logs/$i".log"
  rm -rf response_logs/*

  cat throughput/* >  throughput/total.log
  echo -n $i"," >> logs/up/throughput_time.csv
  awk '{ total += $1; count++ } END { print total }' throughput/total.log >> logs/up/throughput_time.csv
  mv throughput/total.log logs/up/throughput_logs/$i".log"
  rm -rf throughput/*

  rm -rf temp/*


  done
