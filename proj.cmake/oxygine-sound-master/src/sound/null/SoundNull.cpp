#include "SoundNull.h"
namespace oxygine
{
    SoundNull::SoundNull()
    {
    }

    SoundNull::~SoundNull()
    {

    }

    int SoundNull::getDuration() const
    {
        return 0;
    }

    SoundHandle* SoundNull::createSH()
    {
        return 0;
    }
}