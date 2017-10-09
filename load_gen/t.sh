i=400

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
