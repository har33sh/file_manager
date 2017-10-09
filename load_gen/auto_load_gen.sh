time_out="1m"


for i in $(seq 1 20 1000);
do
  timeout $time_out  ./a.out 10.129.23.130 1110 $i 2
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
