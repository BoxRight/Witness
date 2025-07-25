# Witness DSL Syntax Highlighting

This directory contains syntax highlighting files for the Witness Domain Specific Language (DSL) for various code editors.

## Supported Editors

### Visual Studio Code

1. **Install as Extension:**
   - Copy the `package.json`, `language-configuration.json`, and `syntaxes/` folder to a new directory
   - Open VS Code and go to Extensions (Ctrl+Shift+X)
   - Click "..." and select "Install from VSIX..."
   - Package the extension: `vsce package`
   - Install the generated `.vsix` file

2. **Manual Installation:**
   - Copy `syntaxes/witness.tmLanguage.json` to `~/.vscode/extensions/witness-dsl/syntaxes/`
   - Copy `language-configuration.json` to `~/.vscode/extensions/witness-dsl/`
   - Restart VS Code

### Vim/Neovim

1. **Install globally:**
   ```bash
   # Copy to system-wide syntax directory
   sudo cp syntax/witness.vim /usr/share/vim/vim*/syntax/
   
   # Add to filetype detection
   echo "au BufNewFile,BufRead *.wit set filetype=witness" >> ~/.vimrc
   ```

2. **Install for user:**
   ```bash
   # Create user syntax directory
   mkdir -p ~/.vim/syntax/
   
   # Copy syntax file
   cp syntax/witness.vim ~/.vim/syntax/
   
   # Add to filetype detection
   echo "au BufNewFile,BufRead *.wit set filetype=witness" >> ~/.vimrc
   ```

### Sublime Text

1. **Install Package:**
   - Copy `syntaxes/Witness.sublime-syntax` to `~/Library/Application Support/Sublime Text/Packages/User/` (macOS)
   - Or copy to `%APPDATA%\Sublime Text\Packages\User\` (Windows)
   - Or copy to `~/.config/sublime-text/Packages/User/` (Linux)

2. **Manual Installation:**
   - Open Sublime Text
   - Go to Preferences > Browse Packages
   - Create a new folder called "Witness"
   - Copy the syntax file into this folder

## Features

The syntax highlighting supports:

### Keywords
- **Type definitions:** `asset`, `clause`, `subject`, `action`, `object`, `service`, `time`, `authority`
- **Logical operators:** `AND`, `OR`, `NOT`, `IMPLIES`, `EQUIV`
- **Type constraints:** `movable`, `non_movable`, `positive`, `negative`

### System Operations
- `global()`, `domain()`, `litis()`, `meet()`

### Join Operations
- `join()`, `sell()`, `compensation()`, `consideration()`, `forbearance()`, `encumber()`, `access()`, `lien()`, `evidence()`, `argument()`

### Logical Operations
- `oblig()`, `claim()`, `not()`

### Comments
- Line comments: `// comment`
- Block comments: `/* comment */`

### Strings
- Double-quoted strings: `"string content"`

### Numbers
- Integer and decimal numbers

## Color Scheme

The syntax highlighting uses standard editor color schemes:

- **Keywords:** Blue/Purple
- **Functions:** Green
- **Strings:** Orange/Yellow
- **Comments:** Gray
- **Numbers:** Cyan
- **Identifiers:** Default text color

## Example

```wit
// This is a comment
subject alice = "Alice Smith";
action pay = "pay rent", payment;
object rent = "monthly rent", movable;

asset rent_payment = alice, pay, landlord;
clause rent_obligation = oblig(rent_payment) AND NOT claim(rent_payment);

asset contract_valid = global();
```

## Contributing

To add support for additional editors or improve existing syntax highlighting:

1. Fork the repository
2. Create your syntax file
3. Test with sample `.wit` files
4. Submit a pull request

## License

MIT License - feel free to use and modify as needed. 