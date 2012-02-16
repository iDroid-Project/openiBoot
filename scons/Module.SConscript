import SCons

Import("*")

class Module:
	def __init__(self, env, name, sources, dependancies=[]):
		self.env = env
		self.name = name
		self.sources = sources
		self.dependencies = dependancies
		self.appends = []

	def Append(self, **what):
		self.appends.append(what)

	def Depends(self, *what):
		for w in what:
			if SCons.Util.is_List(w):
				self.dependencies += w
			else:
				self.dependencies.append(w)

	def Add(self, env):
		if env['MODULE_SRC'] is None:
			env['MODULE_SRC'] = self.sources.clone()
		else:
			env['MODULE_SRC'] += self.sources

		if env['MODULE_DEPS'] is None:
			env['MODULE_DEPS'] = self.dependencies.clone()
		else:
			env['MODULE_DEPS'] += self.dependencies

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
	env['OBJPREFIX'] = name + '_' + env['OBJPREFIX']
	env.Append(CPPDEFINES=[flag])

	if env['MODULE_SRC'] is not None:
		sources = sources + env['MODULE_SRC']

	deps = []
	if env['MODULE_DEPS'] is not None:
		deps = env['MODULE_DEPS']

	denv = env.Clone()
	denv['OBJSUFFIX'] = '_debug' + denv['OBJSUFFIX']
	denv.Append(CPPDEFINES=['DEBUG'])
	denv.Append(CPPFLAGS=['-g'])

	# Work out filenames
	elf_name = '#' + fname
	delf_name = elf_name + '_debug'
	bin_name = elf_name + '.bin'
	dbin_name = delf_name + '.bin'
	img3_name = elf_name + '.img3'
	dimg3_name = delf_name + '.img3'

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
	Depends(elf, deps)
	delf = denv.Program(delf_name, sources)
	Depends(delf, deps)

	bin = env.Make8900Image(bin_name, elf)
	dbin = denv.Make8900Image(dbin_name, delf)
	if img3template is not None:
		img3 = env.Make8900Image(img3_name, elf+['#mk8900image/%s.img3' % img3template])
		dimg3 = denv.Make8900Image(dimg3_name, delf+['#mk8900image/%s.img3' % img3template])
		Alias(name, img3)
		Alias(name+'D', dimg3)
	else:
		img3 = None
		dimg3 = None
		Alias(name, bin)
		Alias(name+'D', dbin)

	locals()[elf_name] = elf
	locals()[delf_name] = delf
	locals()[bin_name] = bin
	locals()[dbin_name] = dbin
	locals()[img3_name] = img3
	locals()[dimg3_name] = dimg3

	Export([elf_name, bin_name, img3_name,
			delf_name, dbin_name, dimg3_name])

	return elf, bin, img3
env.AddMethod(OpenIBootTarget, "OpenIBootTarget")

env['MODULES'] = {}
env['MODULE_SRC'] = []
env['MODULE_DEPS'] = []
