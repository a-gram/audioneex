
import sys
import os

from audioneex import (Database, Recognizer, AudioSourceFile, AudioBuffer)


def identify(audio):
    """
    Identify the given audio.
    """
    exdir = os.path.dirname(os.path.realpath(__file__))
    dbdir = os.path.join(exdir, "data")
    
    if not os.path.exists(dbdir):
        raise FileNotFoundError(
            "The database could not be found in '%s'." % dbdir
        )
    
    if not os.path.exists(audio):
        raise FileNotFoundError(
            "The file '%s' does not exist." % audio
        )
        
    # The relevant code starts here
    
    db = Database(dbdir)
    db.open(Database.FETCH)
    
    rec = Recognizer(db)
    asource = AudioSourceFile(audio)
    audio = AudioBuffer()

    while True:
        read_samples = asource.readin(audio)
        rec.identify(audio)
        results = rec.get_results()
        if results or not read_samples:
            break;
    
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
    
