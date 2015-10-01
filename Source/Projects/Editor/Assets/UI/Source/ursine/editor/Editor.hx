package ursine.editor;

import haxe.rtti.Meta;

import ursine.controls.*;

import ursine.editor.MenuItemHandler;

///////////////////////////////////////////////////////////////////////////////
// Main Menu Items
///////////////////////////////////////////////////////////////////////////////

import ursine.editor.menus.FileMenu;
import ursine.editor.menus.EditMenu;
import ursine.editor.menus.EntityMenu;
import ursine.editor.menus.DebugMenu;

class Editor {
    public static var instance : Editor = null;

    public var broadcastManager : NativeBroadcastManager;
    public var mainMenu : MainMenu;

    public function new() {
        instance = this;

        broadcastManager = new NativeBroadcastManager( );

        mainMenu = new MainMenu( );

        buildMenus( );

        js.Browser.document
            .querySelector( '#header-toolbar' )
            .appendChild( mainMenu );
    }

    private function buildMenus() {
        var classTypeNames : Array<String> =
            untyped __js__( "Object.keys( $hxClasses )" );

        var menuHandlerType = Type.resolveClass(
            Type.getClassName( MenuItemHandler )
        );

        var handlers : Array<Dynamic> = [ ];

        for (className in classTypeNames) {
            var classType = Type.resolveClass( className );

            var base = Type.getSuperClass( classType );

            if (base == menuHandlerType) {
                var info : Dynamic = { };

                info.type = classType;

                var meta = Meta.getType( classType );

                if (Reflect.hasField( meta, "menuIndex" ))
                    info.index = meta.menuIndex[ 0 ];
                else
                    info.index = Math.POSITIVE_INFINITY;

                handlers.push( info );
            }
        }

        // sort based on index ascending order
        handlers.sort( function(a, b) {
            return a.index - b.index;
        } );

        for (handler in handlers) {
            var statics = Meta.getStatics( handler.type );
            var fields = Reflect.fields( statics );

            for (name in fields) {
                var field = Reflect.field( statics, name );

                // isn't a menu item
                if (!Reflect.hasField( field, "mainMenuItem" ))
                    continue;

                var details : Array<Dynamic> = field.mainMenuItem;

                // levels in the menu
                var levels = StringTools
                    .replace( details[ 0 ], '"', "" )
                    .split( "/" );

                // clickable item that we're creating
                var itemName = levels.pop( );

                // parent menu that this item will be added to
                var parentMenu = populateMenuLevels( levels );

                var item = new MenuItem( );

                item.text = itemName;

                var handler = Reflect.field( handler.type, name );

                item.addEventListener( 'open', function(e) {
                    Reflect.callMethod( handler.type, cast handler, [ e ] );
                } );

                // separator before
                if (details[ 1 ] == true) {
                    parentMenu.appendChild( new MenuSeparator( ) );
                }

                parentMenu.appendChild( item );

                // separator after
                if (details[ 2 ] == true) {
                    parentMenu.appendChild( new MenuSeparator( ) );
                }
            }
        }
    }

    private function populateMenuLevels(levels : Array<String>) : Menu {
        var parent : Menu = mainMenu;

        while (levels.length > 0) {
            var level = levels.shift( );

            var item : MenuItem = parent.findItem( level );

            // item doesn't exist, create it
            if (item == null) {
                item = new MenuItem( );

                item.text = level;

                parent.appendChild( item );
            }

            parent = item.menu;
        }

        return parent;
    }
}