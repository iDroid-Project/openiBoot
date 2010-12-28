import SCons

Import("*")

class Module:
	def __init__(self, env, name, sources, dependancies=[]):
		self.env = env
		self.name = name
		self.sources = sources
		self.dependancies = dependancies
		self.appends = []

	def Append(self, **what):
		self.appends.append(what)

	def Add(self, env):
		if env['MODULE_SRC'] is None:
			env['MODULE_SRC'] = self.sources.clone()
		else:
			env['MODULE_SRC'] += self.sources

		if env['MODULE_DEPS'] is None:
			env['MODULE_DEPS'] = self.dependancies.clone()
		else:
			env['MODULE_DEPS'] += self.dependancies

		for a in self.appends:
			env.Append(**a)

def FindModule(env, name):
	if env['MODULES'] != None and env['MODULES'].has_key(name):
		return env['MODULES'][name]

	return None
env.AddMethod(FindModule, "FindModule")

def CreateModule(env, name, sources, dependancies=[]):
	mod = FindModule(env, name)
	if mod is not None:
		return mod

	mod = Module(env, name, sources, dependancies)
	if env['MODULES'] is None:
		env['MODULES'] = {name: mod}
	else:
		env['MODULES'][name] = mod
		
	return mod
env.AddMethod(CreateModule, "CreateModule")

def AddModule(env, name):
	mod = env.FindModule(name)
	if mod is None:
		print "No such module %s." % name
		return None

	mod.Add(env)
	return mod
env.AddMethod(AddModule, "AddModule")

def AddModules(env, names):
	if not SCons.Util.is_List(names):
		return [AddModule(env, names)]

	return [AddModule(env, name) for name in names]
env.AddMethod(AddModules, "AddModules")

def OpenIBootTarget(env, name, fname, flag, sources, img3template=None):
	env = env.Clone()
	env['OBJPREFIX'] = name + '_'	
	env.Append(CPPDEFINES=[flag])

	if env['MODULE_SRC'] is not None:
		sources = sources + env['MODULE_SRC']

	# Work out filenames
	elf_name = '#' + fname
	bin_name = elf_name + '.bin'
	img3_name = elf_name + '.img3'

	def listify(ls):
		ret = []
		for e in ls:
			if SCons.Util.is_List(e):
				ret += listify(e)
			else:
				ret.append(e)
		return ret
	sources = listify(sources)

	# Add init/sentinal, but make sure first file linked is still first
	# (this means that _start gets run first... kinda important) -- Ricky26
	sources = sources[:1] + ['#init.c'] + sources[1:] + ['#sentinel.c']

	# Add Targets
	elf = env.Program(elf_name, sources)
	bin = env.Make8900Image(bin_name, elf)
	if img3template is not None:
		img3 = env.Make8900Image(img3_name, elf+['#mk8900image/%s.img3' % img3template])
		Alias(name, img3)
	else:
		img3 = None
		Alias(name, bin)

	locals()[elf_name] = elf
	locals()[bin_name] = bin
	locals()[img3_name] = img3
	Export([elf_name, bin_name, img3_name])

	return elf, bin, img3
env.AddMethod(OpenIBootTarget, "OpenIBootTarget")

env['MODULES'] = {}
env['MODULE_SRC'] = []
env['MODULE_DEPS'] = []
