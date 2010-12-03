import subprocess

def GetGitCommit():
	p = subprocess.Popen('git log | head -n1 | cut -b8-14', shell=True, stdout=subprocess.PIPE)
	ret,smth = p.communicate()
	return ret[:-1]
Export('GetGitCommit')
