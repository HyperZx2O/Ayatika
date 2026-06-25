#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "mock_data.h"

static void setSurah(MockSurah *s, int number, const char *name, const char *arabic,
                     const char *meaning, const char *type, int count, const char *ctx) {
    s->number = number;
    strncpy(s->name, name, sizeof(s->name) - 1);
    strncpy(s->arabicName, arabic, sizeof(s->arabicName) - 1);
    strncpy(s->meaning, meaning, sizeof(s->meaning) - 1);
    strncpy(s->revelationType, type, sizeof(s->revelationType) - 1);
    s->ayahCount = count;
    strncpy(s->context, ctx, sizeof(s->context) - 1);
}

static void setAyah(MockAyah *a, int surah, int ayah, const char *arabic,
                    const char *en, const char *bn) {
    a->surahNumber = surah;
    a->ayahNumber = ayah;
    strncpy(a->arabicText, arabic, sizeof(a->arabicText) - 1);
    strncpy(a->translationEn, en, sizeof(a->translationEn) - 1);
    strncpy(a->translationBn, bn, sizeof(a->translationBn) - 1);
    strncpy(a->audioUrl, "", sizeof(a->audioUrl) - 1);
}

void loadMockData(MockAppState *state) {
    /* ── Surahs ── */
    state->surahs = (MockSurah *)malloc(7 * sizeof(MockSurah));
    if (!state->surahs) return;

    setSurah(&state->surahs[0], 1, "Al-Fatiha", "الفاتحة",
             "The Opening", "Meccan", 7,
             "This Meccan surah is the essence of the Quran's message — praise, mercy, and guidance. It is recited in every unit of the Muslim prayer.");
    setSurah(&state->surahs[1], 2, "Al-Baqarah", "البقرة",
             "The Cow", "Medinan", 286,
             "The longest surah in the Quran, revealed in Medina. It covers a wide range of legal, social, and spiritual guidance for the Muslim community.");
    setSurah(&state->surahs[2], 112, "Al-Ikhlas", "الإخلاص",
             "The Sincerity", "Meccan", 4,
             "A concise declaration of God's absolute oneness. The Prophet said this surah equals one-third of the Quran in meaning.");
    setSurah(&state->surahs[3], 113, "Al-Falaq", "الفَلَق",
             "The Daybreak", "Meccan", 5,
             "A prayer seeking refuge from the evil of creation, darkness, and envy. Revealed alongside An-Nas as the two 'protecting' surahs.");
    setSurah(&state->surahs[4], 114, "An-Nas", "النَّاس",
             "Mankind", "Meccan", 6,
             "The final surah of the Quran. A prayer for refuge from the whisperer who retreats, among jinn and humankind.");
    setSurah(&state->surahs[5], 103, "Al-Asr", "العَصْر",
             "The Time", "Meccan", 3,
             "A short but profound surah: all of humanity is in loss except those who believe, do good, and encourage truth and patience.");
    setSurah(&state->surahs[6], 108, "Al-Kawthar", "الكَوْثَر",
             "The Abundance", "Meccan", 3,
             "The shortest surah in the Quran. A promise of abundant blessings to the Prophet and a command to pray and sacrifice.");

    /* ── Ayahs (flat array) ── */
    int total = 0;
    /* Al-Fatiha: 7 ayahs */
    MockAyah ayat[22];

    setAyah(&ayat[total++], 1, 1,
        "بِسْمِ اللَّهِ الرَّحْمَٰنِ الرَّحِيمِ",
        "In the name of Allah, the Most Gracious, the Most Merciful.",
        "শুরু করছি আল্লাহর নামে, যিনি পরম করুণাময়, অসীম দয়ালু।");

    setAyah(&ayat[total++], 1, 2,
        "الْحَمْدُ لِلَّهِ رَبِّ الْعَالَمِينَ",
        "Praise be to Allah, the Lord of all the worlds.",
        "সমস্ত প্রশংসা আল্লাহর, যিনি সমগ্র বিশ্ব জাহানের প্রতিপালক।");

    setAyah(&ayat[total++], 1, 3,
        "الرَّحْمَٰنِ الرَّحِيمِ",
        "The Most Gracious, the Most Merciful.",
        "যিনি পরম করুণাময়, অসীম দয়ালু।");

    setAyah(&ayat[total++], 1, 4,
        "مَالِكِ يَوْمِ الدِّينِ",
        "Master of the Day of Judgment.",
        "যিনি বিচার দিনের মালিক।");

    setAyah(&ayat[total++], 1, 5,
        "إِيَّاكَ نَعْبُدُ وَإِيَّاكَ نَسْتَعِينُ",
        "You alone we worship, and You alone we ask for help.",
        "আমরা শুধুমাত্র তোমারই ইবাদত করি এবং শুধুমাত্র তোমারই সাহায্য প্রার্থনা করি।");

    setAyah(&ayat[total++], 1, 6,
        "اهْدِنَا الصِّرَاطَ الْمُسْتَقِيمَ",
        "Guide us to the straight path.",
        "আমাদেরকে সরল পথ দেখাও।");

    setAyah(&ayat[total++], 1, 7,
        "صِرَاطَ الَّذِينَ أَنْعَمْتَ عَلَيْهِمْ غَيْرِ الْمَغْضُوبِ عَلَيْهِمْ وَلَا الضَّالِّينَ",
        "The path of those You have blessed — not of those who earned Your anger, nor of those who went astray.",
        "তাদের পথ, যাদেরকে তুমি নিয়ামত দান করেছ, তাদের পথ নয় যাদের উপর তোমার গযব নাযিল হয়েছে এবং তাদের পথ নয় যারা পথভ্রষ্ট হয়েছে।");

    /* Al-Baqarah: first 3 + Ayat al-Kursi */
    setAyah(&ayat[total++], 2, 1,
        "الٓمٓ",
        "Alif, Lam, Meem.",
        "আলিফ, লাম, মীম।");

    setAyah(&ayat[total++], 2, 2,
        "ذَٰلِكَ الْكِتَابُ لَا رَيْبَ ۛ فِيهِ ۛ هُدًى لِّلْمُتَّقِينَ",
        "This is the Book about which there is no doubt — a guidance for the righteous.",
        "এটি সেই কিতাব যাতে কোনো সন্দেহ নেই, এটি মুত্তাকীদের জন্য পথনির্দেশ।");

    setAyah(&ayat[total++], 2, 3,
        "الَّذِينَ يُؤْمِنُونَ بِالْغَيْبِ وَيُقِيمُونَ الصَّلَاةَ وَمِمَّا رَزَقْنَاهُمْ يُنفِقُونَ",
        "Those who believe in the unseen, establish prayer, and spend from what We have provided them.",
        "যারা গায়েবে বিশ্বাস করে, নামাজ প্রতিষ্ঠা করে এবং আমি তাদের যা দিয়েছি তা থেকে ব্যয় করে।");

    setAyah(&ayat[total++], 2, 255,
        "اللَّهُ لَا إِلَٰهَ إِلَّا هُوَ الْحَيُّ الْقَيُّومُ ۚ لَا تَأْخُذُهُ سِنَةٌ وَلَا نَوْمٌ ۚ لَهُ مَا فِي السَّمَاوَاتِ وَمَا فِي الْأَرْضِ ۗ مَن ذَا الَّذِي يَشْفَعُ عِندَهُ إِلَّا بِإِذْنِهِ ۚ يَعْلَمُ مَا بَيْنَ أَيْدِيهِمْ وَمَا خَلْفَهُمْ ۖ وَلَا يُحِيطُونَ بِشَيْءٍ مِّنْ عِلْمِهِ إِلَّا بِمَا شَاءَ ۚ وَسِعَ كُرْسِيُّهُ السَّمَاوَاتِ وَالْأَرْضَ ۖ وَلَا يَئُودُهُ حِفْظُهُمَا ۚ وَهُوَ الْعَلِيُّ الْعَظِيمُ",
        "Allah — there is no god but Him, the Ever-Living, the Self-Sustaining. Neither slumber nor sleep overtakes Him. To Him belongs all that is in the heavens and all that is on the earth...",
        "আল্লাহ, তিনি ছাড়া কোনো মা'বুদ নেই, তিনি চিরঞ্জীব, সবকিছুর ধারক। তাঁকে তন্দ্রা বা নিদ্রা স্পর্শ করতে পারে না...");

    /* Al-Ikhlas: 4 ayahs */
    setAyah(&ayat[total++], 112, 1,
        "قُلْ هُوَ اللَّهُ أَحَدٌ",
        "Say: He is Allah, the One.",
        "বলুন: তিনি আল্লাহ, এক।");

    setAyah(&ayat[total++], 112, 2,
        "اللَّهُ الصَّمَدُ",
        "Allah, the Eternal, the Absolute.",
        "আল্লাহ, তিনি অমুখাপেক্ষী।");

    setAyah(&ayat[total++], 112, 3,
        "لَمْ يَلِدْ وَلَمْ يُولَدْ",
        "He neither begets nor is born.",
        "তিনি কাউকে জন্ম দেননি এবং তাঁকেও কেউ জন্ম দেয়নি।");

    setAyah(&ayat[total++], 112, 4,
        "وَلَمْ يَكُن لَّهُ كُفُوًا أَحَدٌ",
        "Nor is there to Him any equivalent.",
        "এবং তাঁর সমতুল্য কেউ নেই।");

    /* An-Nas: first 2 */
    setAyah(&ayat[total++], 114, 1,
        "قُلْ أَعُوذُ بِرَبِّ النَّاسِ",
        "Say: I seek refuge in the Lord of mankind.",
        "বলুন: আমি আশ্রয় চাই মানুষের প্রতিপালকের।");

    setAyah(&ayat[total++], 114, 2,
        "مَلِكِ النَّاسِ",
        "The King of mankind.",
        "মানুষের অধিপতির।");

    /* Al-Kawthar: 3 ayahs */
    setAyah(&ayat[total++], 108, 1,
        "إِنَّا أَعْطَيْنَاكَ الْكَوْثَرَ",
        "Indeed, We have granted you Al-Kawthar (abundant good).",
        "নিশ্চয়ই আমরা আপনাকে আল-কাওসার দান করেছি।");

    setAyah(&ayat[total++], 108, 2,
        "فَصَلِّ لِرَبِّكَ وَانْحَرْ",
        "So pray to your Lord and sacrifice.",
        "অতএব, আপনার প্রতিপালকের উদ্দেশ্যে নামাজ পড়ুন এবং কোরবানি করুন।");

    setAyah(&ayat[total++], 108, 3,
        "إِنَّ شَانِئَكَ هُوَ الْأَبْتَرُ",
        "Indeed, your enemy is the one cut off.",
        "নিশ্চয়ই আপনার শত্রুই নির্বংশ।");

    state->ayahs = (MockAyah *)malloc(total * sizeof(MockAyah));
    if (state->ayahs) {
        memcpy(state->ayahs, ayat, total * sizeof(MockAyah));
        state->totalAyahs = total;
    }

    /* ── Hadith ── */
    state->hadiths = (MockHadith *)malloc(1 * sizeof(MockHadith));
    if (state->hadiths) {
        MockHadith *h = &state->hadiths[0];
        strncpy(h->name, "Sahih Bukhari", sizeof(h->name) - 1);
        strncpy(h->text, "The best among you are those who learn the Quran and teach it.",
                sizeof(h->text) - 1);
        strncpy(h->narrator, "Narrated by Uthman ibn Affan", sizeof(h->narrator) - 1);
        strncpy(h->collection, "Bukhari", sizeof(h->collection) - 1);
        state->totalHadiths = 1;
    }

    /* ── Bookmarks ── */
    (void)state->currentSurah; /* unused in this function */
    /* We store bookmarks inline for now */
    /* ── Prayer Times ── */
    MockPrayerTimes *pt = &state->prayer;
    strncpy(pt->fajrStr, "04:42", sizeof(pt->fajrStr) - 1);
    strncpy(pt->dhuhrStr, "12:14", sizeof(pt->dhuhrStr) - 1);
    strncpy(pt->asrStr, "16:02", sizeof(pt->asrStr) - 1);
    strncpy(pt->maghribStr, "18:48", sizeof(pt->maghribStr) - 1);
    strncpy(pt->ishaStr, "20:04", sizeof(pt->ishaStr) - 1);
    pt->fajr = 4.7f;
    pt->sunrise = 5.9f;
    pt->dhuhr = 12.23f;
    pt->asr = 16.03f;
    pt->maghrib = 18.8f;
    pt->isha = 20.07f;
    pt->prohibitedActive = 0;
    strncpy(pt->prohibitedLabel, "", sizeof(pt->prohibitedLabel) - 1);

    /* ── Nav defaults ── */
    state->currentSurah = 1;
    state->currentAyah = 1;
    state->cursorSurah = 0;
    state->currentScreen = MOCK_SCREEN_DASHBOARD;
    state->previousScreen = MOCK_SCREEN_DASHBOARD;
    state->currentTheme = 0;
    state->focusMode = 0;
    state->showHelp = 0;
    state->dashboardCursor = 0;
    state->lastInputTime = 0.0;
    state->catVisible = 0;
    state->searchResultCount = 0;
    state->searchQuery[0] = '\0';
    strncpy(state->statusMsg, "Press ? for help | j/k to navigate | Enter to open",
            sizeof(state->statusMsg) - 1);
    strncpy(state->language, "en", sizeof(state->language) - 1);
}

MockAyah *findMockAyah(MockAppState *state, int surahNum, int ayahNum) {
    for (int i = 0; i < state->totalAyahs; i++) {
        if (state->ayahs[i].surahNumber == surahNum &&
            state->ayahs[i].ayahNumber == ayahNum)
            return &state->ayahs[i];
    }
    return NULL;
}
