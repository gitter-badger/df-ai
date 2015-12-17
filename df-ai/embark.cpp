#include "ai.h"

Embark::Embark(color_ostream & out, AI *parent) :
    ai(parent)
{
}

Embark::~Embark()
{
}

/*
class DwarfAI
    class RandomEmbark
        attr_accessor :ai
        def initialize(ai)
            @ai = ai
            df.onupdate_register_once('df-ai random_embark') { update } if $AI_RANDOM_EMBARK
        end

        def startup
            # do nothing
        end

        def onupdate_register
            # do nothing
        end

        def onupdate_unregister
            if $AI_RANDOM_EMBARK
                ai.debug 'game over. restarting in 1 minute.'
                ai.timeout_sameview(60) do
                    ai.debug 'restarting.'
                    df.curview.feed_keys(:LEAVESCREEN)

                    df.onupdate_register_once('df-ai restart wait') {
                        next unless df.curview._raw_rtti_classname == 'viewscreen_titlest'

                        unless $NO_QUIT
                            df.curview.breakdown_level = :QUIT
                            next true
                        end

                        # reset
                        $dwarfAI = DwarfAI.new

                        df.onupdate_register_once('df-ai restart') {
                            view = df.curview
                            if view._raw_rtti_classname == 'viewscreen_dwarfmodest'
                                begin
                                    $dwarfAI.onupdate_register
                                    $dwarfAI.startup
                                    view.feed_keys(:D_PAUSE) if df.pause_state
                                rescue Exception
                                    $dwarfAI.debug $!
                                    $dwarfAI.debug $!.backtrace.join("\n")
                                    $dwarfAI.abandon!(view)
                                end
                                true
                            end
                        }

                        true
                    }
                end
            end
        end

        def update
            view = df.curview
            return if not view or view.breakdown_level != :NONE
            case view
            when DFHack::ViewscreenTitlest
                if $RECORD_MOVIE and df.gview.supermovie_on == 0
                    df.gview.supermovie_on = 1
                    df.gview.currentblocksize = 0
                    df.gview.nextfilepos = 0
                    df.gview.supermovie_pos = 0
                    df.gview.supermovie_delayrate = 0
                    df.gview.first_movie_write = 1
                    df.gview.movie_file = "data/movies/df-ai-#{Time.now.to_i}.cmv"
                end

                case view.sel_subpage
                when :None
                    if $AI_RANDOM_EMBARK_WORLD and view.menu_line_id.include?(1)
                        ai.debug 'choosing "Start Game"'
                        view.sel_menu_line = view.menu_line_id.index(1)
                        view.feed_keys(:SELECT)
                    else
                        $AI_RANDOM_EMBARK_WORLD = nil
                        ai.debug 'choosing "New World"'
                        view.sel_menu_line = view.menu_line_id.index(2)
                        view.feed_keys(:SELECT)
                    end
                when :StartSelectWorld
                    if not $AI_RANDOM_EMBARK_WORLD
                        ai.debug 'leaving "Select World" (no save name)'
                        view.feed_keys(:LEAVESCREEN)
                        return
                    end

                    # XXX the memory structures are wrong. Revisit this when they get fixed.
                    l = view.start_savegames.length
                    save = view.continue_savegames.to_a[-l, l].index($AI_RANDOM_EMBARK_WORLD)
                    if save
                        ai.debug "selecting save ##{save}"
                        view.sel_submenu_line = save
                        view.feed_keys(:SELECT)
                    else
                        ai.debug "could not find save named #{$AI_RANDOM_EMBARK_WORLD}"
                        $AI_RANDOM_EMBARK_WORLD = nil
                        view.feed_keys(:LEAVESCREEN)
                    end
                when :StartSelectMode
                    if view.submenu_line_id.include?(0)
                        ai.debug 'choosing "Dwarf Fortress Mode"'
                        view.sel_menu_line = view.submenu_line_id.index(0)
                        view.feed_keys(:SELECT)
                    else
                        ai.debug 'leaving "Select Mode" (no fortress mode available)'
                        $AI_RANDOM_EMBARK_WORLD = nil
                        view.feed_keys(:LEAVESCREEN)
                    end
                end
            when DFHack::ViewscreenNewRegionst
                return if $AI_RANDOM_EMBARK_WORLD

                if not view.unk_33.empty?
                    ai.debug 'leaving world gen disclaimer'
                    view.feed_keys(:LEAVESCREEN)
                elsif df.world.worldgen_status.state == 0
                    ai.debug 'choosing "Generate World"'
                    view.world_size = 1
                    view.feed_keys(:MENU_CONFIRM)
                elsif df.world.worldgen_status.state == 10
                    ai.debug "world gen finished, save name is #{df.world.cur_savegame.save_dir}"
                    $AI_RANDOM_EMBARK_WORLD = df.world.cur_savegame.save_dir
                    view.feed_keys(:SELECT)
                end
            when DFHack::ViewscreenUpdateRegionst
                ai.debug "updating world, goal: #{DwarfAI.timestamp(view.year, view.year_tick)}"
            when DFHack::ViewscreenChooseStartSitest
                if view.finder.finder_state == -1
                    ai.debug 'choosing "Site Finder"'
                    view.feed_keys(:SETUP_FIND)
                    view.finder.options[:DimensionX] = 3
                    view.finder.options[:DimensionY] = 2
                    view.finder.options[:Aquifer] = 0
                    view.finder.options[:River] = 1
                    view.finder.options[:Savagery] = 2
                    view.feed_keys(:SELECT)
                elsif view.finder.search_x == -1
                    if @selected_embark
                        return
                    elsif view.finder.finder_state == 2
                        ai.debug 'choosing "Embark"'
                        view.feed_keys(:LEAVESCREEN)
                        sx, sy = view.location.region_pos.x, view.location.region_pos.y
                        sites = []
                        df.world.world_data.world_width.times do |x|
                            df.world.world_data.world_height.times do |y|
                                s = df.world.world_data.region_map[x][y].finder_rank
                                sites << [x, y] if s >= 10000
                            end
                        end
                        raise 'no good embarks but site finder exited with success' if sites.empty?
                        ai.debug "found sites count: #{sites.length}"
                        site = sites[rand(sites.length)]
                        dx, dy = site[0], site[1]
                        dx -= sx
                        dy -= sy

                        if dx >= 0
                            dx.times do
                                view.feed_keys(:CURSOR_RIGHT)
                            end
                        else
                            (-dx).times do
                                view.feed_keys(:CURSOR_LEFT)
                            end
                        end

                        if dy >= 0
                            dy.times do
                                view.feed_keys(:CURSOR_DOWN)
                            end
                        else
                            (-dy).times do
                                view.feed_keys(:CURSOR_UP)
                            end
                        end

                        @selected_embark = true

                        ai.timeout_sameview(15) do
                            view.feed_keys(:SETUP_EMBARK)
                            view.feed_keys(:SELECT) # dismiss warnings
                        end
                    else
                        ai.debug 'leaving embark selector (no good embarks)'
                        $AI_RANDOM_EMBARK_WORLD = nil
                        view.breakdown_level = :QUIT
                    end
                else
                    ai.debug "searching for a site (#{view.finder.search_x}/#{df.world.world_data.world_width / 16}, #{view.finder.search_y}/#{df.world.world_data.world_height / 16})"
                end
            when DFHack::ViewscreenSetupdwarfgamest
                ai.debug 'choosing "Play Now"'
                view.feed_keys(:SELECT)
                # TODO custom embark loadout
            when DFHack::ViewscreenTextviewerst
                ai.timeout_sameview do
                    ai.debug 'site is ready. disabling minimap.'
                    df.curview.feed_keys(:LEAVESCREEN)
                    df.ui_area_map_width = 3
                    df.ui_menu_width = 3
                    df.standing_orders_job_cancel_announce = 0 unless $DEBUG
                end
                return true
            end
            false
        end

        def status
            ""
        end

        def serialize
            {}
        end
    end
end
*/

// vim: et:sw=4:ts=4