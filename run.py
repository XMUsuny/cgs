import os
import sys
import copy
import subprocess



# available searchers:
# dfs bfs random-state random-path cgs
# nurs:depth nurs:covnew nurs:icnt nurs:cpicnt nurs:rp nurs:md2u nurs:qc 


SOURCE_DIR = os.environ["SOURCE_DIR"]
SANDBOX_DIR = os.environ["SANDBOX_DIR"]
OUTPUT_DIR = os.environ["OUTPUT_DIR"]
KLEE_PATH = SOURCE_DIR + "/klee/build/bin/klee"
MAX_TIME = 3600 * 2
OPTIMIZE = False


# set COV_STATS = True, and use "random-path" on command line,
# to get the coverage for symbolic/concrete branch.
COV_STATS = False


# for cgs
TARGET_BRANCH_NUM = 10
TARGET_BRANCH_UPDATE_INSTS = 300000


pgm_config = {"name": "",
			  "llvm_bc": "",
			  "ubsan_bc": "",
			  "sym_env": ""
			 }


def gen_pgm_config(pgm_name):
	pgm_config_file = SOURCE_DIR + "/benchmark/config.txt"
	with open(pgm_config_file, 'r') as f:
		lines = f.readlines()
		for line in lines:
			items = line.strip().split("##")
			name = items[0]
			if name == pgm_name:
				pgm_cfg = copy.deepcopy(pgm_config)
				pgm_cfg["name"] = name
				pgm_cfg["llvm_bc"] = items[1]
				pgm_cfg["ubsan_bc"] = items[2]
				pgm_cfg["sym_env"] = items[3]
				return pgm_cfg


def gen(pgm_cfg):
	stat_path = SOURCE_DIR + "/stat/"
	if not os.path.exists(stat_path):
		os.system("mkdir -p " + stat_path)

	new_benchmark_path = SOURCE_DIR + "/new_benchmark/"
	if not os.path.exists(new_benchmark_path):
		os.system("mkdir -p " + new_benchmark_path)

	cmd = " ".join(["opt",
					"-load",
					SOURCE_DIR + "/IDA/build/libidapass.so",
					"-ida",
					pgm_cfg["llvm_bc"]])

	os.system(cmd)


def run(pgm_cfg, searcher):

	# env file
	os.system("cp " + SOURCE_DIR + "/test.env" + " " + SANDBOX_DIR + "/test.env")

	# output
	OUTPUT_PROG_DIR = OUTPUT_DIR + '/' + searcher + '/' + pgm_cfg["name"]
	if (os.path.exists(OUTPUT_PROG_DIR)):
		os.system("rm -rf " + OUTPUT_PROG_DIR)
	os.system("mkdir -p " + OUTPUT_DIR + "/" + searcher)

	# llvm bitcode file
	if searcher == "cgs":
		BC_PATH = SOURCE_DIR + "/new_benchmark/" + pgm_cfg["name"] + ".bc"
	else:
		BC_PATH = pgm_cfg["llvm_bc"]
	
	# run in sandbox
	SANDBOX_PROG_DIR = SANDBOX_DIR + "/sandbox-" + searcher + "-" + pgm_cfg["name"]
	if (os.path.exists(SANDBOX_PROG_DIR)):
		os.system("rm -rf " + SANDBOX_PROG_DIR)
	os.system("mkdir -p " + SANDBOX_PROG_DIR)
	os.system("tar -xvf " + SOURCE_DIR + "/sandbox.tgz -C " + SANDBOX_PROG_DIR + " > /dev/null")
	
	# klee basic_command
	basic_cmd = " ".join([KLEE_PATH, 
						"--simplify-sym-indices",
						"--use-forked-solver",
						"--use-cex-cache",
						"--external-calls=all",
						"--switch-type=internal",
						"--libc=uclibc",
						"--posix-runtime",
						"--output-stats",
            			"--max-solver-time=30",
	          			"--max-sym-array-size=4096",
						"--ignore-solver-failures",
						"--only-output-states-covering-new",
						"--dump-states-on-halt=false",
						"--max-memory=4096",
						"--max-memory-inhibit=false",
						"--watchdog",
						"--output-dir=" + OUTPUT_PROG_DIR,
						"--env-file=" + SANDBOX_DIR + "/test.env",
						"--run-in-dir="+ SANDBOX_PROG_DIR,
						"--max-time=" + str(int(MAX_TIME)) + "s",
						"--search=" + searcher,
						])

	# collect branch coverage for motivation
	if COV_STATS:
		basic_cmd = " ".join([basic_cmd, "--cov-stats"]) 

	# klee optimization
	if OPTIMIZE:
		basic_cmd = " ".join([basic_cmd, "--optimize"])

	# hyper-parameter used in cgs
	if searcher == "cgs":
	 	basic_cmd = " ".join([basic_cmd,
	 						"--target-branch-num=" + str(TARGET_BRANCH_NUM),
	 						"--target-branch-update-insts=" + str(TARGET_BRANCH_UPDATE_INSTS)
	 						])

	# add program and symbolic inputs
	basic_cmd = " ".join([basic_cmd, 
						BC_PATH, 
						pgm_cfg["sym_env"]
						])

	# print(basic_cmd)
	os.system(basic_cmd)


def replay_ub(pgm_cfg, searcher):	

	pgm_ub = pgm_cfg["ubsan_bc"][:-3]
	OUTPUT_PROG_DIR = OUTPUT_DIR + '/' + searcher + '/' + pgm_cfg["name"]
	
	ubsan_infos = []
	ubsan_lines = []
	abnormal_lines = []
	for f in os.listdir(OUTPUT_PROG_DIR):
		if f.endswith('.ktest'):

			path = OUTPUT_PROG_DIR + "/" + f
			cmd = " ".join(["timeout 30",
							"klee-replay",
							pgm_ub,
							path])

			p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
			lines = p.stderr.readlines()
			
			for line in lines:
				if "EXIT STATUS: ABNORMAL" in str(line):
					abnormal_lines.append(lines)
					break

			for lines in abnormal_lines:
				for line in lines:
					if "UndefinedBehaviorSanitizer" in str(line) and (str(line) not in ubsan_lines):
						ubsan_lines.append(str(line))
						ubsan_infos.append((f, lines))
	
	for f, lines in ubsan_infos:
		print(f)
		for line in lines:
			print('\t', line)
	for line in ubsan_lines:
		print(line)
	print("In total,", len(ubsan_infos), "ubsan label are triggered")


def main():

	pgm_name = sys.argv[1]
	mode = sys.argv[2]
	if len(sys.argv) > 3:
		searcher = sys.argv[3]
	pgm_cfg = gen_pgm_config(pgm_name)
	
	if mode == "gen":
		gen(pgm_cfg)
	elif mode == "run":
		run(pgm_cfg, searcher)
	elif mode == "replay_ub":
		replay_ub(pgm_cfg, searcher)
	else:
		exit("invalid mode")
		

if __name__ == '__main__':
    main()
