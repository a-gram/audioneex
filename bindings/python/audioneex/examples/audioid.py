
import sys
import os

from audioneex import (Database, Recognizer, AudioSourceFile, AudioBuffer)


def identify(audiofile):
    """
    Identify audio from a file.
    """
    exdir = os.path.dirname(os.path.realpath(__file__))
    dbdir = os.path.join(exdir, "data")
    
    if not os.path.exists(dbdir):
        raise FileNotFoundError(
            "The database could not be found in '%s'." % dbdir
        )
    
    if not os.path.exists(audiofile):
        raise FileNotFoundError(
            "The file '%s' does not exist." % audiofile
        )
        
    # Create a database instance in read mode
    db = Database(dbdir, "db", "read")
    # Create a recognizer connected to the database
    rec = Recognizer(db)
    # Create the audio source (here from a file)
    audio = AudioSourceFile(audiofile)
    
    results = None

    # Read audio from the source until it's identified or consumed
    while audio.read() and results is None:
        results = rec.identify(audio.buffer())
    
    if results:
        for match in results:
            meta = db.get_metadata(match.FID)
            print("-")
            print("FID    : {} ({})".format(match.FID, meta))
            print("Conf.  : {:.0%}".format(match.confidence))
            print("Tp     : {:02d}:{:02d}".format(*_stoms(match.cue_point)))
            print("Score  : {:d}".format(int(match.score)))
            print("Id time: {:.1f}s".format(rec.get_identification_time()))
            print("-")
    else:
        print("No matches found")

    db.close()


def _stoms(s):
    return s//60, s%60


if __name__ == "__main__":

    if len(sys.argv) != 2:
        print("\nUsage: audioid <audiofile>\n")
        sys.exit()
    identify(sys.argv[1])
    
