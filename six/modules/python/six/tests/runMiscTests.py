#!/usr/bin/env python

#
# =========================================================================
# This file is part of six.sicd-python
# =========================================================================
#
# (C) Copyright 2004 - 2016, MDA Information Systems LLC
#
# six.sicd-python is free software; you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this program; If not,
# see <http://www.gnu.org/licenses/>.
#

import os
import random
import subprocess
from subprocess import call

import utils

def getSampleSicdXML():
    samples = os.path.join(utils.findSixHome(), 'six', 'modules', 'c++',
                           'six.sicd', 'tests', 'sample_xml')

    return os.path.join(samples, random.choice(os.listdir(samples)))

def runTests(testDir, testName, *args):
    argList = [utils.executableName(os.path.join(testDir, testName))]
    argList.extend(args)
    result = call(argList, stdout=subprocess.PIPE)

    if result == 0:
        print('Passed')
        return True
    print('Failed')
    return False

def runSICDTests():
    testDir = os.path.join(utils.installPath(), 'tests', 'six.sicd')
    return (runTests(testDir, 'test_add_additional_des', getSampleSicdXML())        and runTests(testDir, 'test_read_sicd_with_extra_des',
        getSampleSicdXML()))


def run():
    return runSICDTests()
