# hope

## about
hope is a lightweight 2d game framework you can use to make games with the [fe](https://github.com/rxi/fe) programming language.
it's more of an SDL2 wrapper than it is a framework, but I like to think of it as a framework

## how to use
like love2d, hope needs to have a folder path passed to it. from there it'll look for main.fe and optionally conf.fe, if it exists.
unsure if you can just drag a folder towards the executable, haven't tried that yet.

## building
the easiest way to build is to clone the repo and open hope.cbp in code::blocks.
it is also possible to use gcc but I haven't tested it.

## api

currently the api is so small I can put it in the readme, but it might be extended with more things.

### general use functions

| function | description |
| :-: | :-: |
| (color r g b a) | sets the current color, all drawing operations will use this color |
| (cls) | clears the screen |
| (pset x y) | draws a pixel |
| (line x1 y1 x2 y2) | draws a line |
| (rectfill x y w h) | draws a filled rectangle |
| (iskeypressed key) | checks if key `key` is pressed |
| (quit) | quits the game |

## math functions

| function | description |
| :-: | :-: |
| (pow x) | returns `x` to the `y` power |
| (sqrt x) | returns the square root of `x` |
| (sin x) | returns the sine of `x` |
| (cos x) | returns the cosine of `x` |
| (% x y) | returns `x` modulo `y` |

## callbacks

| callback | description |
| :-: | :-: |
| (= update (fn (dt) ... )) | main update loop |

## config variables

| variable | description |
| :-: | :-: |
| (= windowTitle "hope") | sets the window's title |
| (= windowSize (cons 600 600)) | sets the window's size |
