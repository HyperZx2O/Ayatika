/* deterministic harness fixture; the app loads live API data. */
#include <string.h>
#include "test_data.h"

static Surah tSurahs[] = {
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

static Ayah tAyahs[] = {
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
    {2, 286, "", "The Messenger has believed in what was revealed to him from his Lord, and so have the believers.", "", ""},
    {3, 190, "", "Indeed, in the creation of the heavens and the earth are signs for those of understanding.", "", ""},
    {3, 159, "", "So by mercy from Allah, you were lenient with them; and if you had been harsh, they would have dispersed.", "", ""},
    {36, 1, "", "Ya Sin. By the wise Quran.", "", ""},
    {36, 58, "", "Peace - a word from a Merciful Lord.", "", ""},
    {55, 1, "", "The Most Merciful taught the Quran.", "", ""},
    {55, 13, "", "So which of the favors of your Lord would you deny?", "", ""},
    {67, 1, "", "Blessed is He in whose hand is dominion, and He is over all things competent.", "", ""},
    {67, 15, "", "It is He who made the earth tame for you - so walk among its slopes and eat of His provision.", "", ""},
    {49, 13, "", "O mankind, indeed We have created you from male and female and made you peoples and tribes that you may know one another.", "", ""},
    {93, 5, "", "And your Lord is going to give you, and you will be satisfied.", "", ""},
    {94, 6, "", "Indeed, with hardship comes ease.", "", ""},
    {2, 153, "", "O you who have believed, seek help through patience and perseverance.", "", ""},
    {2, 152, "", "So remember Me; I will remember you. And be grateful to Me and do not deny Me.", "", ""},
    {31, 14, "", "And We have enjoined upon man care for his parents; his mother carried him in weakness upon weakness.", "", ""},
    {17, 23, "", "And your Lord has decreed that you not worship except Him, and to parents, good treatment.", "", ""},
    {4, 135, "", "O you who have believed, be persistently standing firm in justice, witnesses for Allah.", "", ""},
    {5, 8, "", "O you who have believed, be persistently standing firm for Allah, witnesses in justice.", "", ""},
    {16, 90, "", "Indeed, Allah orders justice and good conduct and giving to relatives.", "", ""},
    {13, 28, "", "Those who have believed and whose hearts are assured by the remembrance of Allah.", "", ""},
};

static Hadith tHadiths[] = {
    {"Hadith 1", "Actions are judged by intentions.", "Umar ibn Al-Khattab", "Bukhari"},
    {"Hadith 2", "The best among you have the best manners.", "Abdullah ibn Amr", "Bukhari"},
};

void loadTestData(AppState *state) {
    state->surahs = tSurahs;
    state->surahCount = sizeof(tSurahs) / sizeof(tSurahs[0]);
    state->ayahs = tAyahs;
    state->totalAyahs = sizeof(tAyahs) / sizeof(tAyahs[0]);
    state->hadiths = tHadiths;
    state->totalHadiths = sizeof(tHadiths) / sizeof(tHadiths[0]);

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
    strncpy(state->prayer.dhuhrStr, "12:30 PM", 15);
    strncpy(state->prayer.asrStr, "4:00 PM", 15);
    strncpy(state->prayer.maghribStr, "6:00 PM", 15);
    strncpy(state->prayer.ishaStr, "7:45 PM", 15);

    state->lastInputTime = 0;
    strncpy(state->language, "en", 7);
}

