Frothing (Feed Reading On The Hoof) is a news aggregator for online news feeds. There are many other news readers available, but some are not available for mobile Linux devices or require many extra libraries to be installed. Frothing tries to fill this gap by creating a fast and easy to use news aggregator for mobile Linux handheld devices

Original repo is here (with pics) -> https://code.google.com/archive/p/frothing/

*Compiling from Source*

The source can be checked out from the repositories by:

`$ git clone https://github.com/IanLawrence/frothing.git`

After unpacking/checking out the code, run the standard autotools commands inside your target:

    ./configure --enable-hildon

    make

    make install

*Dependencies*

On Ubuntu the dependencies can be satisfied by: 

    apt-get install libgtkhtml2-0 libgtkhtml2-dev libxml-perl libxslt1-dev libglade2-0 libglade2-dev libsqlite3-0 libsqlite3-dev libhildondesktop-dev xulrunner

