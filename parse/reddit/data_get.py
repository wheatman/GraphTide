import subprocess
import json
from multiprocessing import Pool
import os
import bz2
import lzma
import zstandard
import io

filelocation = "https://files.pushshift.io/reddit/comments/"

filenames = [
    # 'RC_2005-12.bz2',
    # 'RC_2005-12.bz2',
    # 'RC_2006-01.bz2',
    # 'RC_2006-02.bz2',
    # 'RC_2006-03.bz2',
    # 'RC_2006-04.bz2',
    # 'RC_2006-05.bz2',
    # 'RC_2006-06.bz2',
    # 'RC_2006-07.bz2',
    # 'RC_2006-08.bz2',
    # 'RC_2006-09.bz2',
    # 'RC_2006-10.bz2',
    # 'RC_2006-11.bz2',
    # 'RC_2006-12.bz2',
    # 'RC_2007-01.bz2',
    # 'RC_2007-02.bz2',
    # 'RC_2007-03.bz2',
    # 'RC_2007-04.bz2',
    # 'RC_2007-05.bz2',
    # 'RC_2007-06.bz2',
    # 'RC_2007-07.bz2',
    # 'RC_2007-08.bz2',
    # 'RC_2007-09.bz2',
    # 'RC_2007-10.bz2',
    # 'RC_2007-11.bz2',
    # 'RC_2007-12.bz2',
    # 'RC_2008-01.bz2',
    # 'RC_2008-02.bz2',
    # 'RC_2008-03.bz2',
    # 'RC_2008-04.bz2',
    # 'RC_2008-05.bz2',
    # 'RC_2008-06.bz2',
    # 'RC_2008-07.bz2',
    # 'RC_2008-08.bz2',
    # 'RC_2008-09.bz2',
    # 'RC_2008-10.bz2',
    # 'RC_2008-11.bz2',
    # 'RC_2008-12.bz2',
    # 'RC_2009-01.bz2',
    # 'RC_2009-02.bz2',
    # 'RC_2009-03.bz2',
    # 'RC_2009-04.bz2',
    # 'RC_2009-05.bz2',
    # 'RC_2009-06.bz2',
    # 'RC_2009-07.bz2',
    # 'RC_2009-08.bz2',
    # 'RC_2009-09.bz2',
    # 'RC_2009-10.bz2',
    # 'RC_2009-11.bz2',
    # 'RC_2009-12.bz2',
    # 'RC_2010-01.bz2',
    # 'RC_2010-02.bz2',
    # 'RC_2010-03.bz2',
    # 'RC_2010-04.bz2',
    # 'RC_2010-05.bz2',
    # 'RC_2010-06.bz2',
    # 'RC_2010-07.bz2',
    # 'RC_2010-08.bz2',
    # 'RC_2010-09.bz2',
    # 'RC_2010-10.bz2',
    # 'RC_2010-11.bz2',
    # 'RC_2010-12.bz2',
    # 'RC_2011-01.bz2',
    # 'RC_2011-02.bz2',
    # 'RC_2011-03.bz2',
    # 'RC_2011-04.bz2',
    # 'RC_2011-05.bz2',
    # 'RC_2011-06.bz2',
    # 'RC_2011-07.bz2',
    # 'RC_2011-08.bz2',
    # 'RC_2011-09.bz2',
    # 'RC_2011-10.bz2',
    # 'RC_2011-11.bz2',
    # 'RC_2011-12.bz2',
    # 'RC_2012-01.bz2',
    # 'RC_2012-02.bz2',
    # 'RC_2012-03.bz2',
    # 'RC_2012-04.bz2',
    # 'RC_2012-05.bz2',
    # 'RC_2012-06.bz2',
    # 'RC_2012-07.bz2',
    # 'RC_2012-08.bz2',
    # 'RC_2012-09.bz2',
    # 'RC_2012-10.bz2',
    # 'RC_2012-11.bz2',
    # 'RC_2012-12.bz2',
    # 'RC_2013-01.bz2',
    # 'RC_2013-02.bz2',
    # 'RC_2013-03.bz2',
    # 'RC_2013-04.bz2',
    # 'RC_2013-05.bz2',
    # 'RC_2013-06.bz2',
    # 'RC_2013-07.bz2',
    # 'RC_2013-08.bz2',
    # 'RC_2013-09.bz2',
    # 'RC_2013-10.bz2',
    # 'RC_2013-11.bz2',
    # 'RC_2013-12.bz2',
    # 'RC_2014-01.bz2',
    # 'RC_2014-02.bz2',
    # 'RC_2014-03.bz2',
    # 'RC_2014-04.bz2',
    # 'RC_2014-05.bz2',
    # 'RC_2014-06.bz2',
    # 'RC_2014-07.bz2',
    # 'RC_2014-08.bz2',
    # 'RC_2014-09.bz2',
    # 'RC_2014-10.bz2',
    # 'RC_2014-11.bz2',
    # 'RC_2014-12.bz2',
    # 'RC_2015-01.bz2',
    # 'RC_2015-02.bz2',
    # 'RC_2015-03.bz2',
    # 'RC_2015-04.bz2',
    # 'RC_2015-05.bz2',
    # 'RC_2015-06.bz2',
    # 'RC_2015-07.bz2',
    # 'RC_2015-08.bz2',
    # 'RC_2015-09.bz2',
    # 'RC_2015-10.bz2',
    # 'RC_2015-11.bz2',
    # 'RC_2015-12.bz2',
    # 'RC_2016-01.bz2',
    # 'RC_2016-02.bz2',
    # 'RC_2016-03.bz2',
    # 'RC_2016-04.bz2',
    # 'RC_2016-05.bz2',
    # 'RC_2016-06.bz2',
    # 'RC_2016-07.bz2',
    # 'RC_2016-08.bz2',
    # 'RC_2016-09.bz2',
    # 'RC_2016-10.bz2',
    # 'RC_2016-11.bz2',
    # 'RC_2016-12.bz2',
    # 'RC_2017-01.bz2',
    # 'RC_2017-02.bz2',
    # 'RC_2017-03.bz2',
    # 'RC_2017-04.bz2',
    # 'RC_2017-05.bz2',
    # 'RC_2017-06.bz2',
    # 'RC_2017-07.bz2',
    # 'RC_2017-08.bz2',
    # 'RC_2017-09.bz2',
    # 'RC_2017-10.bz2',
    # 'RC_2017-11.bz2',
    # 'RC_2017-12.xz',
    # 'RC_2018-01.xz',
    # 'RC_2018-02.xz',
    # 'RC_2018-03.xz',
    # 'RC_2018-04.xz',
    # 'RC_2018-05.xz',
    # 'RC_2018-06.xz',
    # 'RC_2018-07.xz',
    # 'RC_2018-08.xz',
    # 'RC_2018-09.xz',
    # 'RC_2018-10.xz',
    # 'RC_2018-10.zst',
    # 'RC_2018-11.zst',
    # 'RC_2018-12.zst',
    #'RC_2019-01.zst',
    #    'RC_2019-02.zst',
    #    'RC_2019-03.zst',
    #    'RC_2019-04.zst',
    #    'RC_2019-05.zst',
    #    'RC_2019-06.zst',
    #    'RC_2019-07.zst',
    #    'RC_2019-08.zst',
    #    'RC_2019-09.zst',
    #    'RC_2019-10.zst',
    #    'RC_2019-11.zst',
    #    'RC_2019-12.zst',
    #'RC_2020-01.zst',
    'RC_2020-02.zst',
    'RC_2020-03.zst',
    'RC_2020-04.zst',
    'RC_2020-05.zst',
    'RC_2020-06.zst',
    'RC_2020-07.zst',
    'RC_2020-08.zst',
    'RC_2020-09.zst',
    'RC_2020-10.zst',
    'RC_2020-11.zst',
    'RC_2020-12.zst',
    'RC_2021-01.zst',
    'RC_2021-02.zst',
    'RC_2021-03.zst',
    'RC_2021-04.zst',
    'RC_2021-05.zst',
    'RC_2021-06.zst'
]


def process_file(fname):
    fname_stripped = fname.split(".")[0]
    if os.path.exists(fname_stripped+".edges"):
        print("output file already exists, skipping ", fname)
        return True
    filepath = filelocation + fname
    if fname.endswith("bz2"):
        subprocess.run("wget " + filepath, shell=True)
        input_file = bz2.open(fname)
    if fname.endswith("xz"):
        subprocess.run("wget " + filepath, shell=True)
        input_file = lzma.open(fname)
    if fname.endswith("zst"):
        subprocess.run("wget " + filepath, shell=True)
        zst_file = open(fname, "rb")
        dctx = zstandard.ZstdDecompressor(max_window_size=2147483648)
        stream_reader = dctx.stream_reader(zst_file)
        input_file = io.TextIOWrapper(stream_reader, encoding='utf-8')
        #ret = subprocess.run("unzstd " + fname, shell=True)
    print("downloaded " + filepath)
    output_filename = fname_stripped+".edges_"
    with open(output_filename, "w") as output_file:
        for line in input_file:
            data = json.loads(line)
            output_file.write(data["author"] + "," + data["subreddit"] +
                              "," + str(data["created_utc"])+"\n")
    if fname.endswith("bz2") or fname.endswith("xz"):
        input_file.close()
    if fname.endswith("zst"):
        zst_file.close()
    subprocess.run("rm -f " + fname_stripped, shell=True)
    subprocess.run("rm -f " + fname, shell=True)
    subprocess.run("mv " + output_filename + " " +
                   output_filename[:-1], shell=True)
    return


#for fname in filenames:
#    process_file(fname)


p = Pool(6)
with p:
    p.map(process_file, filenames)
