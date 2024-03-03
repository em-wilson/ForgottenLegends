#ifndef __ICLANREADER_H__
#define __ICLANREADER_H__

class Clan;

class ClanNotFoundInFileException : public std::exception { };

class IClanReader {
    public:
        virtual std::list<Clan *> loadClans() = 0;
};

#endif