# libscry
A Magic: The Gathering library

Uses Scryfall API to generate card data.
## Dependencies
curl, sqlite3 and rapidjson
## Install
Arch
```
git clone https://github.com/EmperorPenguin18/libscry
cd libscry
makepkg -si
```
Or just install from the [AUR](https://aur.archlinux.org/packages/libscry)

Other Linux
```
git clone https://github.com/EmperorPenguin/libscry
cd libscry
make
make install #as root
```
## Documentation
Available in [docs](docs/)
## Legal
From the API Docs:

**_Use of Scryfall Data_**

_As part of the Wizards of the Coast Fan Content Policy,
Scryfall provides our card data and image database free of charge for the primary
purpose of creating additional Magic software, performing research,
or creating community content (such as videos, set reviews, etc)
about Magic and related products._

_When using Scryfall data, you must adhere to the following guidelines:_

_- You may not use Scryfall logos or use the Scryfall name in a way_
_that implies Scryfall has endorsed you, your work, or your product._  
_- You may not require anyone to make payments, take surveys, agree to subscriptions,_
_rate your content, or create accounts in exchange for access to Scryfall data._  
_- You may not use Scryfall data to create new games, or_
_to imply the information and images are from any other game besides Magic: The Gathering._  

And for images:

**_Image Guidelines_**

_Card images on Scryfall are copyright Wizards of the Coast
(and/or their artist, for very old sets) and they are provided for the
purpose of creating additional Magic software,
or creating community content (such as videos, set reviews, etc) about Magic
and related products._

_When using images from Scryfall, please adhere to the following
guidelines:_

_- Do not cover, crop, or clip off the copyright or artist name on card images._  
_- Do not distort, skew, or stretch card images._  
_- Do not blur, sharpen, desaturate, or color-shift card images._  
_- Do not add your own watermarks, stamps, or logos to card images._  
_- Do not place card images in a way that implies someone other than Wizards of the
Coast created the card or that it is from another game besides Magic: The Gathering._  

_In particular, when using the art_crop:_

_List the artist name and copyright elsewhere in the same interface presenting
the art crop, or use the full card image elsewhere in the same interface.
Users should be able to identify the artist and source of the image somehow._
