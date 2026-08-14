#include <string.h>
#include <stdlib.h>
#include "mock_data.h"

static Surah mockSurahs[] = {
    {1,  "Al-Fatiha", "الفاتحة", "The Opening", "Meccan", 7,
     "The first Surah revealed in full, known as the Mother of the Quran."},
    {2,  "Al-Baqarah", "البقرة", "The Cow", "Medinan", 286,
     "The longest Surah in the Quran, covering law, faith, and history."},
    {103, "Al-Asr", "العصر", "The Declining Day", "Meccan", 3,
     "A short but profound Surah about the loss mankind is in except those who believe."},
    {112, "Al-Ikhlas", "الإخلاص", "The Sincerity", "Meccan", 4,
     "Declares the absolute oneness of Allah. Equivalent to one-third of the Quran."},
    {113, "Al-Falaq", "الفلق", "The Daybreak", "Meccan", 5,
     "A prayer seeking protection from the evil of creation."},
    {114, "An-Nas", "الناس", "Mankind", "Meccan", 6,
     "A prayer seeking refuge in Allah from the whisperings of Shaytan."}
};

static Ayah mockAyahs[] = {
    {1, 1, "بِسْمِ اللَّهِ الرَّحْمَٰنِ الرَّحِيمِ", "In the name of Allah, the Most Gracious, the Most Merciful.", "", ""},
    {1, 2, "الْحَمْدُ لِلَّهِ رَبِّ الْعَالَمِينَ", "All praise and thanks are for Allah, the Lord of all worlds.", "", ""},
    {1, 3, "الرَّحْمَٰنِ الرَّحِيمِ", "The Most Gracious, the Most Merciful.", "", ""},
    {1, 4, "مَالِكِ يَوْمِ الدِّينِ", "The Sovereign of the Day of Judgment.", "", ""},
    {1, 5, "إِيَّاكَ نَعْبُدُ وَإِيَّاكَ نَسْتَعِينُ", "You alone we worship, and You alone we ask for help.", "", ""},
    {103, 1, "وَالْعَصْرِ", "By the declining day,", "", ""},
    {103, 2, "إِنَّ الْإِنْسَانَ لَفِي خُسْرٍ", "Indeed, mankind is in loss,", "", ""},
    {103, 3, "إِلَّا الَّذِينَ آمَنُوا وَعَمِلُوا الصَّالِحَاتِ", "Except those who believe and do righteous deeds.", "", ""},
    {112, 1, "قُلْ هُوَ اللَّهُ أَحَدٌ", "Say, He is Allah, the One.", "", ""},
    {112, 2, "اللَّهُ الصَّمَدُ", "Allah, the Eternal, the Absolute.", "", ""},
    {112, 3, "لَمْ يَلِدْ وَلَمْ يُولَدْ", "He neither begets nor is born.", "", ""},
    {113, 1, "قُلْ أَعُوذُ بِرَبِّ الْفَلَقِ", "Say, I seek refuge in the Lord of the daybreak,", "", ""},
    {113, 2, "مِنْ شَرِّ مَا خَلَقَ", "From the evil of what He has created,", "", ""},
    {114, 1, "قُلْ أَعُوذُ بِرَبِّ النَّاسِ", "Say, I seek refuge in the Lord of mankind,", "", ""},
    {114, 2, "مَلِكِ النَّاسِ", "The King of mankind,", "", ""},
    {114, 3, "إِلَٰهِ النَّاسِ", "The God of mankind,", "", ""},
    {2, 157, "", "Those are the ones upon whom are blessings from their Lord and mercy, and it is they who are the rightly guided.", "", ""},
    {2, 238, "", "Guard strictly the prayers, especially the middle prayer, and stand before Allah, devoutly obedient.", "", ""},
    {2, 255, "", "Allah - there is no deity except Him, the Ever-Living, the Self-Sustaining. Neither drowsiness overtakes Him nor sleep. To Him belongs whatever is in the heavens and whatever is on the earth. Who is it that can intercede with Him except by His permission? He knows what is before them and what will be after them, and they encompass nothing of His knowledge except what He wills. His Kursi extends over the heavens and the earth, and their preservation tires Him not. And He is the Most High, the Most Great.", "", ""},
    {2, 257, "", "Allah is the ally of those who believe; He brings them out from darknesses into the light.", "", ""},
};

static Hadith mockHadiths[] = {
    {"Hadith 1", "Actions are judged by intentions, and every person will get the reward according to what he has intended.",
     "Umar ibn Al-Khattab", "Bukhari"},
    {"Hadith 2", "The best among you are those who have the best manners and character.",
     "Abdullah ibn Amr", "Bukhari"},
};

void loadMockData(AppState *state) {
    state->surahs = mockSurahs;
    state->ayahs = mockAyahs;
    state->totalAyahs = sizeof(mockAyahs) / sizeof(mockAyahs[0]);
    state->hadiths = mockHadiths;
    state->totalHadiths = sizeof(mockHadiths) / sizeof(mockHadiths[0]);

    state->currentSurah = 1;
    state->currentAyah = 1;
    state->currentScreen = SCREEN_DASHBOARD;
    state->previousScreen = SCREEN_DASHBOARD;
    state->searchResultCount = 0;
    state->searchQuery[0] = '\0';

    state->prayer.fajr = 5.0f;
    state->prayer.sunrise = 6.5f;
    state->prayer.dhuhr = 12.5f;
    state->prayer.asr = 16.0f;
    state->prayer.maghrib = 18.0f;
    state->prayer.isha = 19.75f;
    strncpy(state->prayer.fajrStr, "5:00 AM", 15);
    strncpy(state->prayer.sunriseStr, "6:30 AM", 15);
    strncpy(state->prayer.dhuhrStr, "12:30 PM", 15);
    strncpy(state->prayer.asrStr, "4:00 PM", 15);
    strncpy(state->prayer.maghribStr, "6:00 PM", 15);
    strncpy(state->prayer.ishaStr, "7:45 PM", 15);

    state->prayer.prohibitedActive = 0;
    state->prayer.prohibitedLabel[0] = '\0';

    state->isPlayingRecitation = 0;
    state->isNatureSoundOn = 0;
    state->lastInputTime = 0;
    state->catVisible = 0;

    strncpy(state->language, "en", 7);
}
