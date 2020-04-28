
import sys
import os
import glob

from audioneex import (Database, Indexer, AudioSourceFile, 
                       AudioBuffer, AudioProvider)


def fingerprint(audiofile):
    """
    Fingerprint the given audio file(s).
    """        
    exdir = os.path.dirname(os.path.realpath(__file__))
    dbdir = os.path.join(exdir, "data")
    
    if not os.path.exists(dbdir):
        os.mkdir(dbdir)
    
    if not os.path.exists(audiofile):
        raise FileNotFoundError("%s does not exist." % audiofile)

    if os.path.isdir(audiofile):
        audiofiles = glob.glob(os.path.join(audiofile, "**/*.ogg"), 
                               recursive=True)
    else:
        audiofiles = [audiofile]
        
    # Create a database instance and open it for insertions
    db = Database(dbdir, "db", "write")
    # Create an audio provider and connect it to an audio source (file)
    aprovider = AudioProvider(AudioSourceFile())
    # Create an indexer connected to the database and audio provider
    idx = Indexer(db, aprovider)
    # Initialize the starting fingerprint id
    fid = db.get_fp_count() + 1
    # Start a fingerprinting session
    idx.start()
    
    for afile in audiofiles:
        print("Fingerprinting %s" % afile)
        aprovider.audio_source.open(afile)
        idx.index(fid)
        meta = os.path.basename(afile)
        db.put_metadata(fid, meta)
        fid += 1
    
    # ALWAYS close the fingerprinting session when you're done
    idx.end()
    # Close the db if no longer needed (will be done anyway at gc time)
    db.close()

    
if __name__ == "__main__":

    if len(sys.argv) != 2:
        print("\nUsage: audiofp <audiofile | audiodir>\n")
        sys.exit()
    fingerprint(sys.argv[1])
    
