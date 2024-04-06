# Localization Guide

UberWolf contains a localization system that allows easy translation of all UI elements and log messages. This guide will walk you through the process of localizing UberWolf.

1) Grab a copy of the English localization file from the UberWolf repository. The file can be found [here](../UberWolf/lang/en.json).
2) Rename the file to the language you are translating to. For example, if you translate it into French, rename it to 'fr.json'.
3) Open the file in a text editor and translate the strings. The file is a simple JSON file with a key-value pair for each string. The key is an identifier for the string, and the value is the translated string. For example:
```json
{
	"select_game": "Select Game",
	"game_location": "Game Location",
	"unpacking_msg": "Unpacking: {} ... ",
}
```
4) Once you have translated all strings, create a folder named `langs` in the folder containing `UberWolf.exe` and place your file in it. When you start UberWolf, your translation should be available from the `Language` menu.
5) Check if everything is displayed correctly and then submit the translated file to the UberWolf repository as an issue or a pull request. The file will be added to the repository and included in the next release. Please also include if and how you would like to be credited for your translation.

## Important Notes

1) The localization file must be saved in UTF-8 encoding.
2) The value can contain placeholders, such as `{}` in the `unpacking_msg` string. These placeholders can be moved around as needed, but they must be left in the translated string, as they will be replaced with the appropriate value at runtime, e.g., `Unpacking: data.wolf ...`
3) The first three strings in the file are special and, therefore, explained a bit more in detail:
	- `lang_code`: The language code for the language, e.g., `en` for English, `fr` for French, etc.
	- `lang_name`: The name of the language in the language itself, e.g. `English`, `Français`, etc.
	- `language`: The word for "Language" in the language itself, e.g. `Language`, `Langue`, etc. 
```json
{
	"lang_code": "en",
	"lang_name": "English",
	"language": "Language",
	...
}
```
4) Before submitting the file, please make sure that all strings are translated and that it is valid JSON. You can use an online JSON validator, such as [jsonlint](https://jsonlint.com/), to check the file.
