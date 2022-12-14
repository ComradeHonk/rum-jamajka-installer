/* === This file is part of Calamares - <https://calamares.io> ===
 *
 *   SPDX-FileCopyrightText: 2016 Teo Mrnjavac <teo@kde.org>
 *   SPDX-FileCopyrightText: 2017-2019 Adriaan de Groot <groot@kde.org>
 *   SPDX-License-Identifier: GPL-3.0-or-later
 *
 *   Calamares is Free Software: see the License-Identifier above.
 *
 */

#include "LocaleConfiguration.h"
#include "LocaleNames.h"

#include "utils/Logger.h"

#include <QLocale>
#include <QRegularExpression>
#include <QVector>

LocaleConfiguration::LocaleConfiguration()
    : explicit_lang( false )
    , explicit_lc( false )
{
}


LocaleConfiguration::LocaleConfiguration( const QString& localeName, const QString& formatsName )
    : LocaleConfiguration()
{
    lc_numeric = lc_time = lc_monetary = lc_paper = lc_name = lc_address = lc_telephone = lc_measurement
        = lc_identification = formatsName;

    setLanguage( localeName );
}


void
LocaleConfiguration::setLanguage( const QString& localeName )
{
    QString language = localeName.split( '_' ).first();
    m_languageLocaleBcp47 = QLocale( language ).bcp47Name().toLower();
    m_lang = localeName;
}

static LocaleNameParts
updateCountry( LocaleNameParts p, const QString& country )
{
    p.country = country;
    return p;
}

static QPair< int, LocaleNameParts >
identifyBestLanguageMatch( const LocaleNameParts& referenceLocale, QVector< LocaleNameParts >& others )
{
    std::sort( others.begin(),
               others.end(),
               [ & ]( const LocaleNameParts& lhs, const LocaleNameParts& rhs )
               { return referenceLocale.similarity( lhs ) < referenceLocale.similarity( rhs ); } );
    // The best match is at the end
    LocaleNameParts best_match = others.last();
    if ( !( referenceLocale.similarity( best_match ) > LocaleNameParts::no_match ) )
    {
        cDebug() << Logger::SubEntry << "Got no good match for" << referenceLocale.name();
        return { LocaleNameParts::no_match, LocaleNameParts {} };
    }
    else
    {
        cDebug() << Logger::SubEntry << "Got best match for" << referenceLocale.name() << "as" << best_match.name();
        return { referenceLocale.similarity( best_match ), best_match };
    }
}

/** @brief Returns the QString from @p availableLocales that best-matches.
 */
static LocaleNameParts
identifyBestLanguageMatch( const QString& languageLocale,
                           const QStringList& availableLocales,
                           const QString& countryCode )
{
    const QString default_lang = QStringLiteral( "en_US.UTF-8" );

    const LocaleNameParts self = LocaleNameParts::fromName( languageLocale );
    if ( self.isValid() && !availableLocales.isEmpty() )
    {
        QVector< LocaleNameParts > others;
        others.resize( availableLocales.length() );  // Makes default structs
        std::transform( availableLocales.begin(), availableLocales.end(), others.begin(), LocaleNameParts::fromName );

        // Keep track of the best match in various attempts
        int best_score = LocaleNameParts::no_match;
        LocaleNameParts best_match;

        // Check with the unmodified language setting
        {
            auto [ score, match ] = identifyBestLanguageMatch( self, others );
            if ( score >= LocaleNameParts::complete_match )
            {
                return match;
            }
            else if ( score > best_score )
            {
                best_match = match;
            }
        }


        // .. but it might match **better** with the chosen location country Code
        {
            auto [ score, match ] = identifyBestLanguageMatch( updateCountry( self, countryCode ), others );
            if ( score >= LocaleNameParts::complete_match )
            {
                return match;
            }
            else if ( score > best_score )
            {
                best_match = match;
            }
        }

        // .. or better yet with the QLocale-derived country
        {
            const QString localeCountry = LocaleNameParts::fromName( QLocale( languageLocale ).name() ).country;
            auto [ score, match ] = identifyBestLanguageMatch( updateCountry( self, localeCountry ), others );
            if ( score >= LocaleNameParts::complete_match )
            {
                return match;
            }
            else if ( score > best_score )
            {
                best_match = match;
            }
        }

        if ( best_match.isValid() )
        {
            cDebug() << Logger::SubEntry << "Matched best with" << best_match.name();
            return best_match;
        }
    }

    // Else we have an unrecognized or unsupported locale, all we can do is go with
    // en_US.UTF-8 UTF-8. This completes all default language setting guesswork.
    return LocaleNameParts::fromName( default_lang );
}

LocaleConfiguration
LocaleConfiguration::fromLanguageAndLocation( const QString& languageLocale,
                                              const QStringList& availableLocales,
                                              const QString& countryCode )
{
    cDebug() << "Mapping" << languageLocale << "in" << countryCode << "to locale.";
    const auto bestLocale = identifyBestLanguageMatch( languageLocale, availableLocales, countryCode );

    // The following block was inspired by Ubiquity, scripts/localechooser-apply.
    // No copyright statement found in file, assuming GPL v2 or later.
    /*  # It is relatively common for the combination of language and location (as
        # selected on the timezone page) not to identify a supported combined
        # locale.  For example, this happens when the user is a migrant, or when
        # they prefer to use a different language to interact with their computer
        # because that language is better-supported.
        #
        # In such cases, we would like to be able to use a locale reflecting the
        # selected language in LANG for messages, character types, and collation,
        # and to make the other locale categories reflect the selected location.
        # This means that we have to guess at a suitable locale for the selected
        # location, and we do not want to ask yet another locale-related question.
        # Nevertheless, some cases are ambiguous: a user who has asked for the
        # English language and identifies their location as Switzerland will get
        # different numeric representation depending on which Swiss locale we pick.
        #
        # The goal of identifying a reasonable default for migrants makes things
        # easier: it is reasonable to default to French for France despite the
        # existence of several minority languages there, because anyone who prefers
        # those languages will probably already have selected them and won't arrive
        # here.  However, in some cases we're unsure, and in some cases we actively
        # don't want to pick a "preferred" language: selecting either Greek or
        # Turkish as the default language for migrants to Cyprus would probably
        # offend somebody!  In such cases we simply punt to the old behaviour of not
        # setting up a locale reflecting the location, which is suboptimal but is at
        # least unlikely to give offence.
        #
        # Our best shot at general criteria for selecting a default language in
        # these circumstances are as follows:
        #
        #  * Exclude special-purpose (e.g. en_DK) and artificial (e.g. la_AU,
        #    tlh_GB) locales.
        #  * If there is a language specific to or very strongly associated with the
        #    country in question, prefer it unless it has rather few native
        #    speakers.
        #  * Exclude minority languages that are relatively unlikely to be spoken by
        #    migrants who have not already selected them as their preferred language
        #    earlier in the installer.
        #  * If there is an official national language likely to be seen in print
        #    media, road signs, etc., then prefer that.
        #  * In cases of doubt, selecting no default language is safe. */

    // We make a proposed locale based on the UI language and the timezone's country. There is no
    // guarantee that this will be a valid, supported locale (often it won't).
    QString lc_formats;
    const QString combined = QString( "%1_%2" ).arg( bestLocale.language ).arg( countryCode );
    if ( availableLocales.contains( bestLocale.language ) )
    {
        cDebug() << Logger::SubEntry << "Exact formats match for language tag" << bestLocale.language;
        lc_formats = bestLocale.language;
    }
    else if ( availableLocales.contains( combined ) )
    {
        cDebug() << Logger::SubEntry << "Exact formats match for combined" << combined;
        lc_formats = combined;
    }

    if ( lc_formats.isEmpty() )
    {
        QStringList available;
        for ( const QString& line : availableLocales )
        {
            if ( line.contains( QString( "_%1" ).arg( countryCode ) ) )
            {
                available.append( line );
            }
        }
        available.sort();
        if ( available.count() == 1 )
        {
            lc_formats = available.first();
        }
        else
        {
            QMap< QString, QString > countryToDefaultLanguage {
                { "AU", "en" },
                { "CN", "zh" },
                { "DE", "de" },
                { "DK", "da" },
                { "DZ", "ar" },
                { "ES", "es" },
                // Somewhat unclear: Oromo has the greatest number of
                // native speakers; English is the most widely spoken
                // language and taught in secondary schools; Amharic is
                // the official language and was taught in primary
                // schools.
                { "ET", "am" },
                { "FI", "fi" },
                { "FR", "fr" },
                { "GB", "en" },
                // Irish (Gaelic) is strongly associated with Ireland,
                // but nearly all its native speakers also speak English,
                // and migrants are likely to use English.
                { "IE", "en" },
                // India has many languages even though Hindi is known as
                // national language but English is used in all computer
                // and mobile devices.
                { "IN", "en" },
                { "IT", "it" },
                { "MA", "ar" },
                { "MK", "mk" },
                { "NG", "en" },
                { "NL", "nl" },
                { "NZ", "en" },
                { "IL", "he" },
                // Filipino is a de facto version of Tagalog, which is
                // also spoken; English is also an official language.
                { "PH", "fil" },
                { "PK", "ur" },
                { "PL", "pl" },
                { "RU", "ru" },
                // Chinese has more speakers, but English is the "common
                // language of the nation" (Wikipedia) and official
                // documents must be translated into English to be
                // accepted.
                { "SG", "en" },
                { "SN", "wo" },
                { "TR", "tr" },
                { "TW", "zh" },
                { "UA", "uk" },
                { "US", "en" },
                { "ZM", "en" }
            };
            if ( countryToDefaultLanguage.contains( countryCode ) )
            {
                QString combinedLocale
                    = QString( "%1_%2" ).arg( countryToDefaultLanguage.value( countryCode ) ).arg( countryCode );

                for ( const QString& line : availableLocales )
                {
                    if ( line.startsWith( combinedLocale ) )
                    {
                        lc_formats = line;
                        break;
                    }
                }
            }
        }
    }

    // If we cannot make a good choice for a given country we go with the LANG
    // setting, which defaults to en_US.UTF-8 UTF-8 if all else fails.
    return LocaleConfiguration( bestLocale.name(), lc_formats.isEmpty() ? bestLocale.name() : lc_formats );
}


bool
LocaleConfiguration::isEmpty() const
{
    return m_lang.isEmpty() && lc_numeric.isEmpty() && lc_time.isEmpty() && lc_monetary.isEmpty() && lc_paper.isEmpty()
        && lc_name.isEmpty() && lc_address.isEmpty() && lc_telephone.isEmpty() && lc_measurement.isEmpty()
        && lc_identification.isEmpty();
}

/// @brief Sets @p value on @p key in the @p map if @p value is non-empty
static inline void
add_lc( QMap< QString, QString >& map, const char* key, const QString& value )
{
    if ( !value.isEmpty() )
    {
        map.insert( key, value );
    }
}

QMap< QString, QString >
LocaleConfiguration::toMap() const
{
    QMap< QString, QString > map;

    add_lc( map, "LANG", m_lang );
    add_lc( map, "LC_NUMERIC", lc_numeric );
    add_lc( map, "LC_TIME", lc_time );
    add_lc( map, "LC_MONETARY", lc_monetary );
    add_lc( map, "LC_PAPER", lc_paper );
    add_lc( map, "LC_NAME", lc_name );
    add_lc( map, "LC_ADDRESS", lc_address );
    add_lc( map, "LC_TELEPHONE", lc_telephone );
    add_lc( map, "LC_MEASUREMENT", lc_measurement );
    add_lc( map, "LC_IDENTIFICATION", lc_identification );

    return map;
}
