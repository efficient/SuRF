import sys
import os

class bcolors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'

#####################################################################################

def reverseHostName ( email ) :
    name, sep, host = email.partition('@')
    hostparts = host[:-1].split('.')
    r_host = ''
    for part in hostparts :
        r_host = part + '.' + r_host
    return r_host + sep + name

#####################################################################################

if (len(sys.argv) < 3) :
    print bcolors.FAIL + 'Usage:'
    print 'arg 1, key type: randint, timestamp, email' 
    print 'arg 2, distribution: uniform, zipfian, latest' + bcolors.ENDC
    sys.exit()

key_type = sys.argv[1]
distribution = sys.argv[2]

print bcolors.OKGREEN + 'key type = ' + key_type 
print 'distribution = ' + distribution + bcolors.ENDC

ycsb_dir = 'YCSB/bin/'
workload_dir = 'workload_spec/'
output_dir='../workloads/'

email_list = 'email_list.txt'
email_list_size = 27549660
email_keymap_file = output_dir + 'email_keymap.txt'

timestamp_list = 'poisson_timestamps.csv'
timestamp_keymap_file = output_dir + 'timestamp_keymap.txt'

if key_type != 'randint' and key_type != 'timestamp' and key_type != 'email' :
    print bcolors.FAIL + 'Incorrect key_type: please pick from randint and email' + bcolors.ENDC
    sys.exit()

if distribution != 'uniform' and distribution != 'zipfian' and distribution != 'latest' :
    print bcolors.FAIL + 'Incorrect distribution: please pick from uniform, zipfian and latest' + bcolors.ENDC
    sys.exit()

out_ycsb_load = output_dir + 'ycsb_load_' + key_type
out_load_ycsbkey = output_dir + 'load_' + 'ycsbkey'
out_load = output_dir + 'load_' + key_type

cmd_ycsb_load = ycsb_dir + 'ycsb load basic -P ' + workload_dir + 'workloadc_' + key_type + '_' + distribution + ' -s > ' + out_ycsb_load

os.system(cmd_ycsb_load)

#####################################################################################

f_load = open (out_ycsb_load, 'r')
f_load_out = open (out_load_ycsbkey, 'w')
for line in f_load :
    cols = line.split()
    if len(cols) > 2 and cols[0] == "INSERT":
        f_load_out.write (cols[2][4:] + '\n')
f_load.close()
f_load_out.close()

cmd = 'rm -f ' + out_ycsb_load
os.system(cmd)

#####################################################################################

if key_type == 'randint' :
    f_load = open (out_load_ycsbkey, 'r')
    f_load_out = open (out_load, 'w')
    for line in f_load :
        f_load_out.write (line)

elif key_type == 'timestamp' :
    timestamp_keymap = {}
    f_timestamp_keymap = open (timestamp_keymap_file, 'w')

    f_timestamp = open (timestamp_list, 'r')
    timestamps = f_timestamp.readlines()

    f_load_out = open (out_load, 'w')
    f_load = open (out_load_ycsbkey, 'r')
    count = 0
    for line in f_load :
        cols = line.split()
        ts = timestamps[count]
        f_load_out.write (ts)
        f_timestamp_keymap.write (cols[0] + ' ' + ts)
        count += 1
    f_timestamp_keymap.close()

elif key_type == 'email' :
    email_keymap = {}
    f_email_keymap = open (email_keymap_file, 'w')

    f_email = open (email_list, 'r')
    emails = f_email.readlines()

    f_load = open (out_load_ycsbkey, 'r')
    f_load_out = open (out_load, 'w')

    sample_size = len(f_load.readlines())
    gap = email_list_size / sample_size

    f_load.close()
    f_load = open (out_load_ycsbkey, 'r')
    count = 0
    for line in f_load :
        cols = line.split()
        email = reverseHostName(emails[count * gap])
        f_load_out.write (email + '\n')
        f_email_keymap.write (cols[0] + ' ' + email + '\n')
        count += 1
    f_email_keymap.close()

f_load.close()
f_load_out.close()

cmd = 'rm -f ' + out_load_ycsbkey
os.system(cmd)
