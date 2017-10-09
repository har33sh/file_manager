time_out="5m"
for i in $(seq 10 5 400);
do
  #Uploads
  timeout $time_out taskset -c 3,4,5,6 ./a.out 10.129.23.130 1111 $i 1

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


  #Downloads
  rm -rf  /home/ghost/Downloads/Data/*
  cp /a.txt /home/ghost/Downloads/Data/

  timeout $time_out taskset -c 3,4,5,6 ./a.out 10.129.23.130 1111 $i 2

  cat response_logs/* >  response_logs/total.log
  echo -n $i"," >> logs/down/response_time.csv
  awk '{ total += $1; count++ } END { print total/count }' response_logs/total.log >> logs/down/response_time.csv
  mv response_logs/total.log logs/down/response_logs/$i".log"
  rm -rf response_logs/*

  cat throughput/* >  throughput/total.log
  echo -n $i"," >> logs/down/throughput_time.csv
  awk '{ total += $1; count++ } END { print total }' throughput/total.log >> logs/down/throughput_time.csv
  mv throughput/total.log logs/down/throughput_logs/$i".log"
  rm -rf throughput/*

  rm -rf temp/*

done

for i in $(seq 10 5 400);
  do
  #Up + Downloads
  rm -rf  /home/ghost/Downloads/Data/*
  cp /a.txt /home/ghost/Downloads/Data/

  timeout $time_out taskset -c 3,4,5,6  ./a.out 10.129.23.130 1111 $i 3

  cat response_logs/* >  response_logs/total.log
  echo -n $i"," >> logs/up_down/response_time.csv
  awk '{ total += $1; count++ } END { print total/count }' response_logs/total.log >> logs/up_down/response_time.csv
  mv response_logs/total.log logs/up_down/response_logs/$i".log"
  rm -rf response_logs/*

  cat throughput/* >  throughput/total.log
  echo -n $i"," >> logs/up_down/throughput_time.csv
  awk '{ total += $1; count++ } END { print total }' throughput/total.log >> logs/up_down/throughput_time.csv
  mv throughput/total.log logs/up_down/throughput_logs/$i".log"
  rm -rf throughput/*

  rm -rf temp/*

  done
