package xgui;
import java.util.*;
public interface xiDimBrowser
{
public abstract Vector<xiDimParameter> getParameters();
public abstract Vector<xiDimCommand> getCommands();
public abstract void addInfoHandler(xiDimParameter parameter, xiUserInfoHandler infohandler);
public abstract void removeInfoHandler(xiDimParameter parameter, xiUserInfoHandler infohandler);
public abstract void sleep(int seconds);
}