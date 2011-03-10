
#PBS -l nodes=1,walltime=00:01:00
#PBS -N APPTest_2
#PBS -o APP.result.2
#PBS -e APP.error.2
#PBS -V

num_samples=3

# change this to your directory.
cd /hivehomes/tiwari/schedular
lamboot
rm -rf temp_result.2
rm -rf timings.2
for i in $(seq 1 1 ${num_samples}); do
    mpirun -np 1 ./application 9 >> temp_result.2;
done

# process the results
cat temp_result.2 | awk '{print $2}' | sort -g | head -1 > timings.2

touch done.iter.2

