/*
Copyright (c) 2014-15 Ableton AG, Berlin

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include "StyleSetProps.hpp"

#include "Convert.hpp"
#include "Log.hpp"
#include "Property.hpp"
#include "StyleEngine.hpp"

namespace aqt
{
namespace stylesheets
{

namespace
{

PropertyMap* nullProperties()
{
  static PropertyMap sNullPropertyMap;
  return &sNullPropertyMap;
}

} // anon namespace

StyleSetProps::StyleSetProps(const UiItemPath& path, StyleEngine* pEngine)
  : mpEngine(pEngine)
  , mPath(path)
  , mpProperties(nullProperties())
{
  loadProperties();
}

StyleSetProps* StyleSetProps::nullStyleSetProps()
{
  static StyleSetProps sNullStyleSetProps{{}, nullptr};
  return &sNullStyleSetProps;
}

bool StyleSetProps::isValid() const
{
  return !mpProperties->empty();
}

bool StyleSetProps::isSet(const QString& key) const
{
  return mpProperties->find(key) != mpProperties->end();
}

bool StyleSetProps::getImpl(Property& prop, const QString& key) const
{
  PropertyMap::const_iterator it = mpProperties->find(key);
  if (it != mpProperties->end()) {
    prop = it->second;
    return true;
  }

  if (mpEngine) {
    styleSheetsLogWarning() << "Property " << key.toStdString() << " not found ("
                            << pathToString(mPath) << ")";
    Q_EMIT mpEngine->exception(QString::fromLatin1("propertyNotFound"),
                               QString::fromLatin1("Property '%1' not found (%2)")
                                 .arg(key, QString::fromStdString(pathToString(mPath))));
  }

  return false;
}

QVariant StyleSetProps::get(const QString& key) const
{
  Property prop;
  getImpl(prop, key);

  if (prop.mValues.size() == 1) {
    auto conv = convertProperty<QString>(prop.mValues[0]);
    if (conv) {
      return QVariant::fromValue(*conv);
    }
  } else if (prop.mValues.size() > 1) {
    QVariantList result;
    for (const auto& propValue : prop.mValues) {
      auto conv = convertProperty<QString>(propValue);
      if (conv) {
        result.push_back(conv.get());
      }
    }

    return result;
  }

  return QVariant();
}

QVariant StyleSetProps::values(const QString& key) const
{
  Property prop;
  getImpl(prop, key);

  if (prop.mValues.size() == 1) {
    return convertValueToVariant(prop.mValues[0]);
  }

  return convertValueToVariantList(prop.mValues);
}

QColor StyleSetProps::color(const QString& key) const
{
  return lookupProperty<QColor>(key);
}

QFont StyleSetProps::font(const QString& key) const
{
  return lookupProperty<QFont>(key);
}

double StyleSetProps::number(const QString& key) const
{
  return lookupProperty<double>(key);
}

bool StyleSetProps::boolean(const QString& key) const
{
  return lookupProperty<bool>(key);
}

QString StyleSetProps::string(const QString& key) const
{
  return lookupProperty<QString>(key);
}

QUrl StyleSetProps::url(const QString& key) const
{
  Property prop;
  auto url = lookupProperty<QUrl>(prop, key);

  if (mpEngine) {
    auto baseUrl = prop.mSourceLoc.mSourceLayer == 0 ? mpEngine->defaultStyleSheetSource()
                                                     : mpEngine->styleSheetSource();
    return mpEngine->resolveResourceUrl(baseUrl, url);
  }

  return url;
}
void StyleSetProps::loadProperties()
{
  if (mpEngine) {
    mpProperties = mpEngine->properties(mPath);
    Q_EMIT propsChanged();
  } else {
    mpProperties = nullProperties();
  }
}

} // namespace stylesheets
} // namespace aqt
