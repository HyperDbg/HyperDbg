import os
import sys
import re
import subprocess

VCXPROJ_TEMPLATE = '''
<Project DefaultTargets="Build" ToolsVersion="12.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <ItemGroup>
    <ProjectConfiguration Include="Debug|Win32">
      <Configuration>Debug</Configuration>
      <Platform>Win32</Platform>
    </ProjectConfiguration>

    <ProjectConfiguration Include="Debug|x64">
      <Configuration>Debug</Configuration>
      <Platform>x64</Platform>
    </ProjectConfiguration>
  </ItemGroup>

  <PropertyGroup>
    <LinkIncremental>false</LinkIncremental>
    <OutDir>.\</OutDir>
    <IntDir>.\Obj\</IntDir>
  </PropertyGroup>

  <PropertyGroup>
    <ConfigurationType>Application</ConfigurationType>
    <PlatformToolset>v141</PlatformToolset>
  </PropertyGroup>

  <ItemGroup>
    <ClCompile Include="%(file_c)s" />
  </ItemGroup>

  <ItemGroup>
    <ClInclude Include="%(file_h)s" />
  </ItemGroup>

  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.default.props" />
  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.props" />
  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.Targets" />
</Project>
'''

MSBUILD_CMD_TEMPLATE = 'msbuild %(file_vcxproj)s /p:configuration=%(configuration)s /p:platform=%(platform)s /p:platformtoolset=%(platformtoolset)s'
PDBEX_CMD_TEMPLATE   = '..\\..\\Bin\\x86\\Release\\pdbex.exe "*" "%(file_pdb)s" -o "%(file_h)s" -t "%(file_c)s" -g "%(symbol_suffix)s"'

OUTPUT_DIRECTORY = 'Output'

VERBOSITY_LEVEL = 0 # 0, 1, 2


def test_create(file_pdb, file_h, file_c, symbol_suffix):
	command = PDBEX_CMD_TEMPLATE % {
		'file_pdb'      : file_pdb,
		'file_h'        : file_h,
		'file_c'        : file_c,
		'symbol_suffix' : symbol_suffix
		}

	if VERBOSITY_LEVEL >= 1:
		print '    ' + command

	subprocess.call(command)


def test_get_platform(file_h):
	platform = 'Win32'

	with open(file_h) as f:
		line_counter = 0
		for line in f:
			line_counter += 1

			#
			# Platform is specified at 3rd line.
			#

			if line_counter == 3:
				if '64' in line:
					platform = 'x64'

				break

	return platform


def test_compile(file_vcxproj, file_c, file_h, platform):
	with open(file_vcxproj, 'w+') as f:
		f.write(VCXPROJ_TEMPLATE % {
				'file_c'      : file_c,
				'file_h'      : file_h
			})

	command = MSBUILD_CMD_TEMPLATE % {
			'file_vcxproj'    : file_vcxproj,
			'configuration'   : 'debug',
			'platform'        : platform,
			'platformtoolset' : 'v142'
		}

	if VERBOSITY_LEVEL >= 1:
		print '    ' + command

	fnull = open(os.devnull, 'w')
	result = subprocess.call(command, stdout=fnull, stderr=fnull)

	if result != 0:
		raise Exception('Compilation error')


def test_run(file_exe):
	p = subprocess.Popen(file_exe, stdout=subprocess.PIPE)
	result = p.communicate()[0]

	if '[!]' in result:
		raise Exception('Test failed:\n' + result)


def process_pdb(file_pdb):
	#
	# Filename without extension.
	#

	try:
		os.chdir(OUTPUT_DIRECTORY)
	except:
		os.mkdir(OUTPUT_DIRECTORY)
		os.chdir(OUTPUT_DIRECTORY)

	filename = os.path.splitext(os.path.basename(file_pdb))[0]

	if (re.search('\\\\[0-9A-Z]{33}\\\\', file_pdb)):
		#
		# We're most likely dumping from symbol path
		# (ie. S:\Symbols\ntdll.pdb\10DC95804D2C4756947338A43F573BAD2\ntdll.pdb)
		#
		# Make the filename as 10DC95804D2C4756947338A43F573BAD2_ntdll
		#
		filename     = os.path.splitext(file_pdb)[0]
		filename     = filename.rsplit('\\', 2)[1] + '_' + filename.rsplit('\\', 2)[2]

	file_c       = filename + '.c'
	file_h       = filename + '.h'
	file_exe     = filename + '.exe'
	file_vcxproj = filename + '.vcxproj'
	
	symbol_suffix  = '_'

	print 'Processing "%s"' % file_pdb

	try:
		if VERBOSITY_LEVEL > 0:
			print '  Extracting data'
		test_create(file_pdb, file_h, file_c, symbol_suffix)

		platform = test_get_platform(file_h)

		if VERBOSITY_LEVEL > 0:
			print '  Compiling for platform "%s"' % platform
		test_compile(file_vcxproj, file_c, file_h, platform)

		if VERBOSITY_LEVEL > 0:
			print '  Testing'
		test_run(file_exe)
	except Exception as e:
		print e

	os.chdir('..')


def process_dir(pdb_dir):
	for root, directories, filenames in os.walk(pdb_dir):
		for filename in filenames: 
			file_pdb = os.path.join(root, filename)
			process_pdb(file_pdb)


def main():
	import argparse
	parser = argparse.ArgumentParser()
	parser.add_argument('pdbs', type=str, nargs='*', help='directory which contains PDB files, or PDB file')
	parser.add_argument('-v', '--verbose', action='store_true', help='increase output verbosity')
	parser.add_argument('-d', '--debug', action='store_true', help='even more verbosity')
	parser.add_argument('-c', '--clean', action='store_true', help='clean all files')

	args = parser.parse_args()

	global VERBOSITY_LEVEL

	if args.verbose:
		VERBOSITY_LEVEL = 1

	if args.debug:
		VERBOSITY_LEVEL = 2

	if args.clean:
		import shutil
		
		try:
			shutil.rmtree(OUTPUT_DIRECTORY)
		except:
			pass
		
		return

	if not hasattr(args, 'pdbs'):
		parser.print_help()
		return
	
	for pdb in args.pdbs:
		pdb = os.path.abspath(pdb)

		if os.path.isfile(pdb):
			process_pdb(pdb)
		elif os.path.isdir(pdb):
			process_dir(pdb)
		else:
			print 'Error: %s is not a directory or file' % pdb


if __name__ == '__main__':
	main()
