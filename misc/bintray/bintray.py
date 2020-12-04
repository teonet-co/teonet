#! /usr/bin/env python

#
# TVH bintray tool, compatible with both python2 and python3
#

import os
import sys
import json
import base64
import traceback
import re
try:
    # Python 3
    import urllib.request as urllib
    from urllib.parse import urlencode
except ImportError:
    # Python 2
    import urllib2 as urllib
    from urllib import urlencode

def env(key):
    if key in os.environ: return os.environ[key]
    return None

DEBUG=False

BINTRAY_API='https://bintray.com/api/v1'
BINTRAY_USER=env('BINTRAY_USER')
BINTRAY_PASS=env('BINTRAY_PASS')
BINTRAY_REPO=env('BINTRAY_REPO')
BINTRAY_COMPONENT=env('BINTRAY_COMPONENT')
BINTRAY_ORG=env('BINTRAY_ORG')
BINTRAY_PACKAGE='libteonet'

PACKAGE_DESC='Teonet network library'

deb_distrib = {
    'ubuntu20.04': 'focal',
    'ubuntu18.04': 'bionic',
    'ubuntu16.04': 'xenial',
    'debian10': 'buster',
    'debian9': 'stretch',
    }
class Response(object):
    def __init__(self, response):
        self.url = response.geturl()
        self.code = response.getcode()
        self.reason = response.msg
        self.headers = response.info()
        if 'Content-type' in self.headers and self.headers['Content-type'] == 'application/json':
          self.body = json.loads(response.read())
        else:
          self.body = response.read()

class Bintray(object):

    def __init__(self, path, headers=None):
        self._headers = headers or {}
        self._path = path or []
        a = '%s:%s' % (BINTRAY_USER, BINTRAY_PASS)
        self._auth = b'Basic ' + base64.b64encode(a.encode('utf-8'))

    def opener(self):
        if DEBUG:
            return urllib.build_opener(urllib.HTTPSHandler(debuglevel=1))
        else:
            return urllib.build_opener()

    def _push(self, data, binary=None, method='PUT'):
        content_type = 'application/json'
        if binary:
          content_type = 'application/binary'
        else:
          data = json.dumps(data)
        opener = self.opener()
        path = self._path
        if path[0] != '/': path = '/' + path
        request = urllib.Request(BINTRAY_API + path, data=data)
        request.add_header('Content-Type', content_type)
        request.add_header('Authorization', self._auth)
        request.get_method = lambda: method
        try:
            r = Response(opener.open(request))
        except urllib.HTTPError as e:
            r = Response(e)
        return r

    def get(self, binary=None):
        return self._push(None, method='GET')

    def delete(self, binary=None):
        return self._push(None, method='DELETE')

    def put(self, data, binary=None):
        return self._push(data, binary)
    
    def post(self, data):
        print("POST: ", data)
        return self._push(data, method='POST')

def error(lvl, msg, *args):
    sys.stderr.write(msg % args + '\n')
    sys.exit(lvl)

def info(msg, *args):
    print('BINTRAY: ' + msg % args)

def do_upload(*args):
    if len(args) < 2: error(1, 'upload [url] [file]')
    bpath, file = args[0], args[1]
    data = open(file, 'rb').read()
    resp = Bintray(bpath).put(data, binary=1)
    if resp.code != 200 and resp.code != 201:
        if resp.code == 409:
            error(0, 'HTTP WARNING "%s" %s %s', resp.url, resp.code, resp.reason)
        error(10, 'HTTP ERROR "%s" %s %s', resp.url, resp.code, resp.reason)

def get_ver(version):
    if version.find('-') > 0:
        version, git = version.split('-', 1)
    else:
        git = None
    try:
      major, minor, rest = version.split('.', 2)
    except:
      major, minor = version.split('.', 1)
      rest = ''
    return (major, minor, rest, git)

def get_path(version, repo):
    major, minor, rest, git = get_ver(version)
    if int(major) >= 4 and int(minor) & 1 == 0:
        if repo in ['fedora', 'centos', 'rhel'] and git.find('~') <= 0:
            return '%s.%s-release' % (major, minor)
        return '%s.%s' % (major, minor)
    return 't'

def get_component(version):
    major, minor, rest, git = get_ver(version)
    if int(major) >= 4 and int(minor) & 1 == 0:
        if git and git.find('~') > 0:
            return 'stable-%s.%s' % (major, minor)
        return 'release-%s.%s' % (major, minor)
    return 'unstable'

def get_repo(filename, hint=None):
    if hint: return hint
    if BINTRAY_REPO: return BINTRAY_REPO
    name, ext = os.path.splitext(filename)
    if ext == '.deb':
        if name.find('_ubuntu') > 0:
            return 'ubuntu'
        elif name.find('_debian') > 0:
            return 'debian'
    elif ext == '.rpm':
        if name.find('.centos') > 0:
            return 'centos'
        elif name.find('.fc') > 0:
            return 'fedora'
        elif name.find('.opensuse') > 0:
            return 'opensuse'
    else:
        return 'tar'

def rpmversion(name):
    ver = type('',(object,),{})()
    rpmbase, ver.arch = name.rsplit('.', 1)
    rpmname, rpmversion = rpmbase.rsplit('-', 1)
    rpmname, rpmversion2 = rpmname.rsplit('-', 1)
    rpmversion = rpmversion2 + '-' + rpmversion
    rpmver1, rpmver2 = rpmversion.split('-', 1)
    rpmversion, ver.dist = rpmver2.split('.', 1)
    ver.version = rpmver1 + '-' + rpmversion
    return ver

def get_bintray_params(filename, hint=None):
    filename = filename.strip()
    basename = os.path.basename(filename)
    name, ext = os.path.splitext(basename)
#    print('name: ', name)
#    print('ext: ', ext)
    args = type('',(object,),{})()
    args.org = BINTRAY_ORG
    args.repo = get_repo(basename, hint)
#    print('args.repo: ', args.repo)
    args.package = BINTRAY_PACKAGE
    extra = []
    if (args.repo == 'debian') or (args.repo == 'ubuntu') :
        debbase, debarch = name.rsplit('.', 1)
#        print('debbase: ', debbase)
#        print('debarch: ', debarch)
        try:
            debname, debversion = debbase.split('_', 1)
#            print('debname: ', debname)
        except:
            debname, debversion = debbase.split('-', 1)
#            print('debname: ', debname)
        debversion, debdistro = debversion.rsplit('_', 1)
#        print('debversion: ', debversion)
#        print('debdistro: ', deb_distrib[debdistro])
        args.version = debversion
        args.path = 'pool/' + get_path(debversion, args.repo) + '/' + args.package
        extra.append('deb_component=' + (BINTRAY_COMPONENT or get_component(debversion)))
        extra.append('deb_distribution=' + deb_distrib[debdistro])
        extra.append('deb_architecture=' + debarch)
    elif (args.repo == 'fedora') or (args.repo == 'centos') or (args.repo == 'opensuse') :
        rpmver = rpmversion(name)
        args.version = rpmver.version
        args.path = 'linux/' + get_path(rpmver.version, args.repo) + \
                    '/' + rpmver.dist + '/' + rpmver.arch
    else:
        return ('tar', args, extra)
    extra = ';'.join(extra)
    if extra: extra = ';' + extra
    return (basename, args, extra)

def getall (path):
    files = []
    for x in os.listdir(path):
        x = os.path.join(path, x)
        if os.path.isdir(x): files += getall(x)
        else: files.append(x)
        print(x)
    return files


def do_publish(*args):
#    return
#    if len(args) < 1: error(1, 'upload [file with the file list]')
#    if not DEBUG:
#        branches = os.popen('git branch --contains HEAD').readlines()
#        ok = 0
#        for b in branches:
#            if b[0] == '*':
#                b = b[1:]
#            b = b.strip()
#            if b == 'master' or b.startswith('release/'):
#                ok = 1
#                break
#        if not ok:
#            info('BINTRAY upload - invalid branches\n%s', branches)
#            sys.exit(0)
#    files = open(args[0]).readlines()
    files = getall(args[0]) 
    args = None
    for file in files:
        try:
            basename, args, extra = get_bintray_params(file)
            print("BASENAME:", basename)
            if (basename == 'tar') :
                continue
            hint = args.repo
            break
        except:
            pass
    if not args:
        for file in files:
            try:
                basename, args, extra = get_bintray_params(file)
            except:
                traceback.print_exc()
    bpath = '/packages/%s/%s/%s/versions' % \
            (BINTRAY_ORG, args.repo, BINTRAY_PACKAGE)
    data = { 'name': args.version, 'desc': PACKAGE_DESC }

    resp = Bintray(bpath).post(data)
    if resp.code != 200 and resp.code != 201 and resp.code != 409:
        error(10, 'Version %s/%s: HTTP ERROR %s %s',
                  args.repo, args.version, resp.code, resp.reason)
    info('Version %s/%s created', args.repo, args.version)
    for file in files:
        file = file.strip()
        basename, args, extra = get_bintray_params(file)
        if (basename == 'tar') :
            continue
        pub = 1
        if "-dirty" in basename.lower():
            pub = 0
        bpath = '/content/%s/%s/%s/%s/%s/%s%s;publish=%s' % \
                (args.org, args.repo, args.package, args.version,
                 args.path, basename, extra, pub)
        data = open(file, 'rb').read()
        resp = Bintray(bpath).put(data, binary=1)
        if resp.code != 200 and resp.code != 201:
            error(10, 'File %s (%s): HTTP ERROR "%s" %s',
                      file, bpath, resp.code, resp.reason)
        else:
            info('File %s: uploaded', file)

def get_versions(repo, package):
    bpath = '/packages/%s/%s/%s' % (BINTRAY_ORG, repo, package)
    resp = Bintray(bpath).get()
    if resp.code != 200 and resp.code != 201:
        error(10, ' %s/%s: HTTP ERROR %s %s',
               repo, package, resp.code, resp.reason)
    return resp.body

def get_files(repo, package, unpublished=0):
    bpath = '/packages/%s/%s/%s/files?include_unpublished=%d' % (BINTRAY_ORG, repo, package, unpublished)
    resp = Bintray(bpath).get()
    if resp.code != 200 and resp.code != 201:
        error(10, ' %s/%s: HTTP ERROR %s %s',
               repo, package, resp.code, resp.reason)
    return resp.body

def delete_file(repo, file):
    bpath = '/content/%s/%s/%s' % (BINTRAY_ORG, repo, urllib.quote(file))
    resp = Bintray(bpath).delete()
    if resp.code != 200 and resp.code != 201:
        error(10, ' %s/%s: HTTP ERROR %s %s',
               repo, file, resp.code, resp.reason)

def delete_up_to_count(repo, files, max_count, auxfcn=None):
    files.sort(key=lambda x: x['sortkey'], reverse=True)
    key = ''
    count = 0
    for f in files:
      a, b = f['sortkey'].split('*')
      if a == key:
        if count > max_count:
          info('delete %s', f['path'])
          if auxfcn:
              auxfcn(repo, f['path'])
          else:
              delete_file(repo, f['path'])
        else:
          info('keep %s', f['path'])
        count += 1
      else:
        key = a
        count = 0

def do_unknown(*args):
    r = 'Please, specify a valid command:\n'
    for n in globals():
        if n.startswith('do_') and n != 'do_unknown':
            r += '  ' + n[3:] + '\n'
    error(1, r[:-1])

def this_parse(this_string):
    name_ver_build = re.split('-|_|\-', this_string)
    platform = re.split('[.]', name_ver_build[3])
    os = str(platform[0] + '.' + platform[1])
    print(name_ver_build[0])
    print(name_ver_build[1])
    print(name_ver_build[2])
    print(os)
    print(platform[2])

def test_filename():
    FILES=[
        "libtest-v0.0.6.tar.gz",
        "libtest_0.0.3-14_debian10.amd64.deb",
        "libtest_0.0.3-14_debian9.amd64.deb",
        "libtest_0.0.3-14_ubuntu16.04.amd64.deb",
        "libtest_0.0.3-14_ubuntu18.04.amd64.deb",
        "libtest_0.0.3-14_ubuntu20.04.amd64.deb",
        "libtest_0.0.3-14_ubuntu20.04.amd64.deb",
        "libtest-0.0.3-14.el7.7.centos.x86_64.rpm",
        "libtest-0.0.3-14.el8.0.centos.x86_64.rpm",
        "libtest-0.0.3-14.fc32.x86_64.rpm",
        "libtest-0.0.3-14.opensuse15.2.x86_64.rpm"

    ]
    from pprint import pprint
    for f in FILES:
        basename, args, extra = get_bintray_params(f)
        if (basename == 'tar') :
            print("TARGZ!!!!!")
            continue
        print('\n')
        print('BASENAME:', basename)
        print('EXTRA:', extra)
        pprint(vars(args), indent=2)

def main(argv):
    global DEBUG
    if len(argv) > 1 and argv[1] == '--test-filename':
        return test_filename()
    if not BINTRAY_USER or not BINTRAY_PASS:
        error(2, 'No credentals')
    if len(argv) > 1 and argv[1] == '--debug':
        DEBUG=1
        argv.pop(0)
    cmd = 'do_' + (len(argv) > 1 and argv[1] or 'unknown')
    if cmd in globals():
        globals()[cmd](*argv[2:])
    else:
        do_unknown()

if __name__ == "__main__":
    main(sys.argv)

