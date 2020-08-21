#include <3ds.h>
#include <citro2d.h>
#include <cstdio>
#include <cstdarg>
#include <string>
#include <cstring>
#include <fstream>
#include <vector>

#include <stdio.h>

#include "data.h"
#include "gfx.h"
#include "ui.h"
#include "util.h"
#include "fs.h"
#include "sys.h"

#include "smdh.h"

static ui::menu mainMenu, titleMenu, backupMenu, nandMenu, nandBackupMenu, folderMenu, sharedMenu;

int state = MAIN_MENU, prev = MAIN_MENU;

//Stolen 3Dbrew descriptions
static const std::string sharedDesc[] =
{
    "主页菜单尝试在启动期间打开此存档，如果 FS：OpenArchive 不返回错误主页菜单后启动系统传输应用程序。主页菜单实际上并不使用此存档，只是检查它是否存在。",
    "NAND JPEG/MPO 文件和 phtcache.bin 从相机应用程序存储在这里。这也包含上传数据.dat。",
    "声音应用程序中的 NAND M4A 文件存储在这里。",
    "适用于用于通知的 SpotPass 内容存储。",
    "包含 idb. dat， idbt. dat， gamecoin. dat， ubll. lst， CFL_DB. dat， CFL_OldDB. dat.这些文件包含 Miis 和一些与播放使用记录相关的数据（包括缓存的 ICN 数据）。",
    "包含 bashotorya. dat 和 bashotorya2. dat 。",
    "主页菜单的 SpotPass 内容数据存储。",
    "包含 版本列表.dat ，由主页菜单用于添加 7.0.0-13 的软件更新通知。"
};

namespace ui
{
    void loadTitleMenu()
    {
        titleMenu.reset();
        titleMenu.multiSet(true);
        for(unsigned i = 0; i < data::titles.size(); i++)
        {
            if(data::titles[i].getFav())
                titleMenu.addOpt("♥ " + util::toUtf8(data::titles[i].getTitle()), 320);
            else
                titleMenu.addOpt(util::toUtf8(data::titles[i].getTitle()), 320);
        }

        titleMenu.adjust();
    }

    void loadNandMenu()
    {
        nandMenu.reset();
        for(unsigned i = 0; i < data::nand.size(); i++)
            nandMenu.addOpt(util::toUtf8(data::nand[i].getTitle()), 320);
    }

    void prepMenus()
    {
        mainMenu.addOpt("Titles", 0);
        mainMenu.addOpt("系统Titles", 0);
        mainMenu.addOpt("共享追加数据", 0);
        mainMenu.addOpt("重新加载应用列表", 0);
        mainMenu.addOpt("修改游戏币", 0);
        mainMenu.addOpt("退出", 0);

        //Title menu
        loadTitleMenu();
        loadNandMenu();

        backupMenu.addOpt("保存存档", 0);
        backupMenu.addOpt("删除保存存档", 0);
        backupMenu.addOpt("追加数据", 0);
        backupMenu.addOpt("删除追加数据", 0);
        backupMenu.addOpt("返回", 0);

        nandBackupMenu.addOpt("系统存档", 0);
        nandBackupMenu.addOpt("追加数据", 0);
        nandBackupMenu.addOpt("BOSS追加数据", 0);
        nandBackupMenu.addOpt("返回", 0);

        sharedMenu.addOpt("E0000000", 0);
        sharedMenu.addOpt("F0000001", 0);
        sharedMenu.addOpt("F0000002", 0);
        sharedMenu.addOpt("F0000009", 0);
        sharedMenu.addOpt("F000000B", 0);
        sharedMenu.addOpt("F000000C", 0);
        sharedMenu.addOpt("F000000D", 0);
        sharedMenu.addOpt("F000000E", 0);
    }

    void drawTopBar(const std::string& info)
    {
        C2D_DrawRectSolid(0, 0, 0.5f, 400, 16, 0xFF505050);
        C2D_DrawRectSolid(0, 17, 0.5f, 400, 1, 0xFF1D1D1D);
        gfx::drawText(info, 4, 0, 0xFFFFFFFF);
    }

    void stateMainMenu(const uint32_t& down, const uint32_t& held)
    {
        mainMenu.handleInput(down, held);

        if(down & KEY_A)
        {
            switch(mainMenu.getSelected())
            {
                case 0:
                    //do a cart check first
                    data::cartCheck();
                    if(!data::titles.empty())
                        state = TITLE_MENU;
                    else
                        ui::showMessage("没有发现Titles，请重新加载Titles！");
                    break;

                case 1:
                    state = SYS_MENU;
                    break;

                case 2:
                    state = SHRD_MENU;
                    break;

                case 3:
                    remove("/JKSV/titles");
                    remove("/JKSV/nand");
                    data::loadTitles();
                    data::loadNand();
                    loadTitleMenu();
                    break;

                case 4:
                    util::setPC();
                    break;

                case 5:
                    sys::run = false;
                    break;
            }
        }
        else if(down & KEY_B)
            sys::run = false;

        gfx::frameBegin();
        gfx::frameStartTop();
        drawTopBar("JKSM - 简体中文汉化版");
        mainMenu.draw(40, 78, 0xFFFFFFFF, 320, false);
        gfx::frameStartBot();
        gfx::frameEnd();
    }

    void stateTitleMenu(const uint32_t& down, const uint32_t& held)
    {
        data::cartCheck();

        //Kick back if empty
        if(data::titles.empty())
        {
            state = MAIN_MENU;
            return;
        }

        //Much needed Jump button
        static ui::button jumpTo("跳转", 0, 208, 320, 32);
        //Dump all button
        static ui::button dumpAll("储存所有存档", 0, 174, 320, 32);
        //Blacklist button
        static ui::button bl("添加到黑名单 \ue002", 0, 140, 320, 32);
        //Selected Dump
        static ui::button ds("储存已选择 \ue003", 0, 106, 320, 32);
        //Favorite
        static ui::button fav("添加到喜爱 \ue005", 0, 72, 320, 32);

        titleMenu.handleInput(down, held);

        touchPosition p;
        hidTouchRead(&p);

        jumpTo.update(p);
        dumpAll.update(p);
        bl.update(p);
        ds.update(p);
        fav.update(p);

        if(down & KEY_A)
        {
            data::curData = data::titles[titleMenu.getSelected()];

            state = BACK_MENU;
        }
        else if(down & KEY_B)
        {
            titleMenu.setSelected(0);
            state = MAIN_MENU;
        }
        else if(down & KEY_X || bl.getEvent() == BUTTON_RELEASED)
        {
            std::string confString = "你确定要添加'" + util::toUtf8(data::titles[titleMenu.getSelected()].getTitle()) + "'到黑名单吗？";
            if(confirm(confString))
                data::blacklistAdd(data::titles[titleMenu.getSelected()]);
        }
        else if(down & KEY_Y || ds.getEvent() == BUTTON_RELEASED)
        {
            for(unsigned i = 0; i < titleMenu.getCount(); i++)
            {
                if(titleMenu.multiIsSet(i) && fs::openArchive(data::titles[i], ARCHIVE_USER_SAVEDATA, false))
                {
                    util::createTitleDir(data::titles[i], ARCHIVE_USER_SAVEDATA);
                    std::u16string outpath = util::createPath(data::titles[i], ARCHIVE_USER_SAVEDATA) + util::toUtf16(util::getDateString(util::DATE_FMT_YMD));
                    FSUSER_CreateDirectory(fs::getSDMCArch(), fsMakePath(PATH_UTF16, outpath.data()), 0);
                    outpath += util::toUtf16("/");

                    fs::backupArchive(outpath);
                    fs::closeSaveArch();
                }

                if(titleMenu.multiIsSet(i) && fs::openArchive(data::titles[i], ARCHIVE_EXTDATA, false))
                {
                    util::createTitleDir(data::titles[i], ARCHIVE_EXTDATA);
                    std::u16string outpath = util::createPath(data::titles[i], ARCHIVE_EXTDATA) + util::toUtf16(util::getDateString(util::DATE_FMT_YMD));
                    FSUSER_CreateDirectory(fs::getSDMCArch(), fsMakePath(PATH_UTF16, outpath.data()), 0);
                    outpath += util::toUtf16("/");

                    fs::backupArchive(outpath);
                    fs::closeSaveArch();
                }
            }
        }
        else if(down & KEY_R || fav.getEvent() == BUTTON_RELEASED)
        {
            if(data::titles[titleMenu.getSelected()].getFav())
                data::favRem(data::titles[titleMenu.getSelected()]);
            else
                data::favAdd(data::titles[titleMenu.getSelected()]);
        }
        else if(jumpTo.getEvent() == BUTTON_RELEASED)
        {
            char16_t getChar = util::toUtf16(util::getString("请输入要跳转的字母", false))[0];
            if(getChar != 0x00)
            {
                unsigned i;
                if(data::titles[0].getMedia() == MEDIATYPE_GAME_CARD)
                    i = 1;
                else
                    i = 0;

                for( ; i < titleMenu.getCount(); i++)
                {
                    if(std::tolower(data::titles[i].getTitle()[0]) == getChar)
                    {
                        titleMenu.setSelected(i);
                        break;
                    }
                }
            }
        }
        else if(dumpAll.getEvent() == BUTTON_RELEASED)
        {
            fs::backupAll();
        }

        gfx::frameBegin();
        gfx::frameStartTop();
        drawTopBar("选择一个Titles");
        titleMenu.draw(40, 24, 0xFFFFFFFF, 320, false);
        gfx::frameStartBot();
        data::titles[titleMenu.getSelected()].drawInfo(8, 8);
        jumpTo.draw();
        dumpAll.draw();
        bl.draw();
        ds.draw();
        fav.draw();
        gfx::frameEnd();
    }

    void prepFolderMenu(data::titleData& dat, const uint32_t& mode)
    {
        std::u16string path = util::createPath(dat, mode);

        fs::dirList bakDir(fs::getSDMCArch(), path);

        folderMenu.reset();
        folderMenu.addOpt("新建", 0);

        for(unsigned i = 0; i < bakDir.getCount(); i++)
            folderMenu.addOpt(util::toUtf8(bakDir.getItem(i)), 320);

        folderMenu.adjust();
    }

    void stateBackupMenu(const uint32_t& down, const uint32_t& held)
    {
        backupMenu.handleInput(down, held);

        if(down & KEY_A)
        {
            switch(backupMenu.getSelected())
            {
                case 0:
                    if(fs::openArchive(data::curData, ARCHIVE_USER_SAVEDATA, true))
                    {
                        util::createTitleDir(data::curData, ARCHIVE_USER_SAVEDATA);
                        prepFolderMenu(data::curData, ARCHIVE_USER_SAVEDATA);
                        prev  = BACK_MENU;
                        state = FLDR_MENU;
                    }
                    break;

                case 1:
                    if(confirm(std::string("您确定要删除此游戏的保存数据吗？\n这是不可逆转的！")) && fs::openArchive(data::curData, ARCHIVE_USER_SAVEDATA, true))
                    {
                        FSUSER_DeleteDirectoryRecursively(fs::getSaveArch(), fsMakePath(PATH_ASCII, "/"));
                        fs::commitData(ARCHIVE_USER_SAVEDATA);
                        fs::closeSaveArch();
                    }
                    break;

                case 2:
                    if(fs::openArchive(data::curData, ARCHIVE_EXTDATA, true))
                    {
                        util::createTitleDir(data::curData, ARCHIVE_EXTDATA);
                        prepFolderMenu(data::curData, ARCHIVE_EXTDATA);
                        prev  = BACK_MENU;
                        state = FLDR_MENU;
                    }
                    break;

                case 3:
                    {
                        std::string confStr = "您确定要删除'" + util::toUtf8(data::curData.getTitle()) + "'的追加保存数据吗？";
                        if(confirm(confStr))
                        {
                            FS_ExtSaveDataInfo del = { MEDIATYPE_SD, 0, 0, data::curData.getExtData(), 0 };

                            Result res = FSUSER_DeleteExtSaveData(del);
                            if(R_SUCCEEDED(res))
                                showMessage("追加保存数据已删除。");
                        }
                    }
                    break;

                case 4:
                    backupMenu.setSelected(0);
                    state = TITLE_MENU;
                    break;
            }
        }
        else if(down & KEY_B)
        {
            backupMenu.setSelected(0);
            state = TITLE_MENU;
        }

        gfx::frameBegin();
        gfx::frameStartTop();
        drawTopBar(util::toUtf8(data::curData.getTitle()));
        backupMenu.draw(40, 82, 0xFFFFFFFF, 320, false);
        gfx::frameStartBot();
        gfx::frameEnd();
    }

    void stateNandMenu(const uint32_t& down, const uint32_t& held)
    {
        nandMenu.handleInput(down, held);

        if(down & KEY_A)
        {
            data::curData = data::nand[nandMenu.getSelected()];
            state = SYS_BAKMENU;
        }
        else if(down & KEY_B)
        {
            nandMenu.setSelected(0);
            state = MAIN_MENU;
        }

        gfx::frameBegin();
        gfx::frameStartTop();
        drawTopBar("请选择一个NAND Title");
        nandMenu.draw(40, 24, 0xFFFFFFFF, 320, false);
        gfx::frameStartBot();
        data::nand[nandMenu.getSelected()].drawInfo(8, 8);
        gfx::frameEnd();
    }

    void stateNandBack(const uint32_t& down, const uint32_t& held)
    {
        nandBackupMenu.handleInput(down, held);

        if(down & KEY_A)
        {
            switch(nandBackupMenu.getSelected())
            {
                case 0:
                    if(fs::openArchive(data::curData, ARCHIVE_SYSTEM_SAVEDATA, true))
                    {
                        util::createTitleDir(data::curData, ARCHIVE_SYSTEM_SAVEDATA);
                        prepFolderMenu(data::curData, ARCHIVE_SYSTEM_SAVEDATA);
                        prev  = SYS_BAKMENU;
                        state = FLDR_MENU;
                    }
                    break;

                case 1:
                    if(fs::openArchive(data::curData, ARCHIVE_EXTDATA, true))
                    {
                        util::createTitleDir(data::curData, ARCHIVE_EXTDATA);
                        prepFolderMenu(data::curData, ARCHIVE_EXTDATA);
                        prev  = SYS_BAKMENU;
                        state = FLDR_MENU;
                    }
                    break;

                case 2:
                    if(fs::openArchive(data::curData, ARCHIVE_BOSS_EXTDATA, true))
                    {
                        util::createTitleDir(data::curData, ARCHIVE_BOSS_EXTDATA);
                        prepFolderMenu(data::curData, ARCHIVE_BOSS_EXTDATA);
                        prev  = SYS_BAKMENU;
                        state = FLDR_MENU;
                    }
                    break;

                case 3:
                    nandBackupMenu.setSelected(0);
                    state = SYS_MENU;
                    break;
            }
        }
        else if(down & KEY_B)
        {
            nandBackupMenu.setSelected(0);
            state = SYS_MENU;
        }

        gfx::frameBegin();
        gfx::frameStartTop();
        drawTopBar(util::toUtf8(data::curData.getTitle()));
        nandBackupMenu.draw(40, 88, 0xFFFFFFFF, 320, false);
        gfx::frameStartBot();
        gfx::frameEnd();
    }

    void stateFolderMenu(const uint32_t& down, const uint32_t& held)
    {
        folderMenu.handleInput(down, held);

        int sel = folderMenu.getSelected();

        if(down & KEY_A)
        {
            //New
            if(sel == 0)
            {
                std::u16string newFolder;
                if(held & KEY_L)
                    newFolder = util::toUtf16(util::getDateString(util::DATE_FMT_YDM));
                else if(held & KEY_R)
                    newFolder = util::toUtf16(util::getDateString(util::DATE_FMT_YMD));
                else
                    newFolder = util::safeString(util::toUtf16(util::getString("输入新文件夹名称", true)));

                if(!newFolder.empty())
                {
                    std::u16string fullPath = util::createPath(data::curData, fs::getSaveMode()) + newFolder;
                    FSUSER_CreateDirectory(fs::getSDMCArch(), fsMakePath(PATH_UTF16, fullPath.data()), 0);
                    fullPath += util::toUtf16("/");

                    fs::backupArchive(fullPath);

                    prepFolderMenu(data::curData, fs::getSaveMode());
                }
            }
            else
            {
                sel--;

                fs::dirList titleDir(fs::getSDMCArch(), util::createPath(data::curData, fs::getSaveMode()));
                std::string confStr = "是否覆盖'" + util::toUtf8(titleDir.getItem(sel)) + "'?";
                if(ui::confirm(confStr))
                {
                    std::u16string fullPath = util::createPath(data::curData, fs::getSaveMode()) + titleDir.getItem(sel);

                    //Del
                    FSUSER_DeleteDirectoryRecursively(fs::getSDMCArch(), fsMakePath(PATH_UTF16, fullPath.data()));

                    //Recreate
                    FSUSER_CreateDirectory(fs::getSDMCArch(), fsMakePath(PATH_UTF16, fullPath.data()), 0);

                    fullPath += util::toUtf16("/");
                    fs::backupArchive(fullPath);
                }
            }
        }
        else if(down &  KEY_Y && sel != 0)
        {
            sel--;
            fs::dirList titleDir(fs::getSDMCArch(), util::createPath(data::curData, fs::getSaveMode()));
            std::string confStr = "你确定要恢复'" + util::toUtf8(titleDir.getItem(sel)) + "'吗?";
            if(confirm(confStr))
            {
                std::u16string restPath = util::createPath(data::curData, fs::getSaveMode()) + titleDir.getItem(sel) + util::toUtf16("/");

                //Wipe root
                FSUSER_DeleteDirectoryRecursively(fs::getSaveArch(), fsMakePath(PATH_ASCII, "/"));

                //Restore from restPath
                fs::restoreToArchive(restPath);
            }
        }
        else if(down & KEY_X && sel != 0)
        {
            sel--;
            fs::dirList titleDir(fs::getSDMCArch(), util::createPath(data::curData, fs::getSaveMode()));
            std::string confStr = "你确定要删除'" + util::toUtf8(titleDir.getItem(sel)) + "'吗?";
            if(confirm(confStr))
            {
                std::u16string delPath = util::createPath(data::curData, fs::getSaveMode()) + titleDir.getItem(sel);

                FSUSER_DeleteDirectoryRecursively(fs::getSDMCArch(), fsMakePath(PATH_UTF16, delPath.data()));

                prepFolderMenu(data::curData, fs::getSaveMode());
            }
        }
        else if(down & KEY_SELECT)
        {
            advModePrep();
            state = ADV_MENU;
        }
        else if(down & KEY_B)
        {
            folderMenu.setSelected(0);
            fs::closeSaveArch();
            state = prev;
        }

        gfx::frameBegin();
        gfx::frameStartTop();
        drawTopBar("选择一个文件夹");
        folderMenu.draw(40, 24, 0xFFFFFFFF, 320, false);
        gfx::frameStartBot();
        gfx::drawText("\ue000 = 选择\n\ue003 = 恢复\n\ue002 = 删除\n\ue001 = 返回\nSelect = 文件模式", 16, 16, 0xFFFFFFFF);
        gfx::frameEnd();
    }

    void stateSharedSelect(const uint32_t& down, const uint32_t& held)
    {
        sharedMenu.handleInput(down, held);
        if(down & KEY_A)
        {
            switch(sharedMenu.getSelected())
            {
                case 0:
                    {
                        data::titleData e0;
                        e0.setExtdata(0xE0000000);
                        e0.setTitle((char16_t *)"E0000000");
                        if(fs::openArchive(e0, ARCHIVE_SHARED_EXTDATA, true))
                        {
                            data::curData = e0;
                            util::createTitleDir(e0, ARCHIVE_SHARED_EXTDATA);
                            ui::prepFolderMenu(e0, ARCHIVE_SHARED_EXTDATA);
                            prev = SHRD_MENU;
                            state = FLDR_MENU;
                        }
                    }
                    break;

                case 1:
                    {
                        data::titleData f1;
                        f1.setExtdata(0xF0000001);
                        f1.setTitle((char16_t *)"F0000001");
                        if(fs::openArchive(f1, ARCHIVE_SHARED_EXTDATA, true))
                        {
                            data::curData = f1;
                            util::createTitleDir(f1, ARCHIVE_SHARED_EXTDATA);
                            ui::prepFolderMenu(f1, ARCHIVE_SHARED_EXTDATA);
                            prev = SHRD_MENU;
                            state = FLDR_MENU;
                        }
                    }
                    break;

                case 2:
                    {
                        data::titleData f2;
                        f2.setExtdata(0xF0000002);
                        f2.setTitle((char16_t *)"F0000002");
                        if(fs::openArchive(f2, ARCHIVE_SHARED_EXTDATA, true))
                        {
                            data::curData = f2;
                            util::createTitleDir(f2, ARCHIVE_SHARED_EXTDATA);
                            ui::prepFolderMenu(f2, ARCHIVE_SHARED_EXTDATA);
                            prev = SHRD_MENU;
                            state = FLDR_MENU;
                        }
                    }
                    break;

                case 3:
                    {
                        data::titleData f9;
                        f9.setExtdata(0xF0000009);
                        f9.setTitle((char16_t *)"F0000009");
                        if(fs::openArchive(f9, ARCHIVE_SHARED_EXTDATA, true))
                        {
                            data::curData = f9;
                            util::createTitleDir(f9, ARCHIVE_SHARED_EXTDATA);
                            ui::prepFolderMenu(f9, ARCHIVE_SHARED_EXTDATA);
                            prev = SHRD_MENU;
                            state = FLDR_MENU;
                        }
                    }
                    break;

                case 4:
                    {
                        data::titleData fb;
                        fb.setExtdata(0xF000000B);
                        fb.setTitle((char16_t *)"F000000B");
                        if(fs::openArchive(fb, ARCHIVE_SHARED_EXTDATA, true))
                        {
                            data::curData = fb;
                            util::createTitleDir(fb, ARCHIVE_SHARED_EXTDATA);
                            ui::prepFolderMenu(fb, ARCHIVE_SHARED_EXTDATA);
                            prev = SHRD_MENU;
                            state = FLDR_MENU;
                        }
                    }
                    break;

                case 5:
                    {
                        data::titleData fc;
                        fc.setExtdata(0xF000000C);
                        fc.setTitle((char16_t *)"F000000C");
                        if(fs::openArchive(fc, ARCHIVE_SHARED_EXTDATA, true))
                        {
                            data::curData = fc;
                            util::createTitleDir(fc, ARCHIVE_SHARED_EXTDATA);
                            ui::prepFolderMenu(fc, ARCHIVE_SHARED_EXTDATA);
                            prev = SHRD_MENU;
                            state = FLDR_MENU;
                        }
                    }
                    break;

                case 6:
                    {
                        data::titleData fd;
                        fd.setExtdata(0xF000000D);
                        fd.setTitle((char16_t *)"F000000D");
                        if(fs::openArchive(fd, ARCHIVE_SHARED_EXTDATA, true))
                        {
                            data::curData = fd;
                            util::createTitleDir(fd, ARCHIVE_SHARED_EXTDATA);
                            ui::prepFolderMenu(fd, ARCHIVE_SHARED_EXTDATA);
                            prev = SHRD_MENU;
                            state = FLDR_MENU;
                        }
                    }
                    break;

                case 7:
                    {
                        data::titleData fe;
                        fe.setExtdata(0xF000000E);
                        fe.setTitle((char16_t *)"F000000E");
                        if(fs::openArchive(fe, ARCHIVE_SHARED_EXTDATA, true))
                        {
                            data::curData = fe;
                            util::createTitleDir(fe, ARCHIVE_SHARED_EXTDATA);
                            ui::prepFolderMenu(fe, ARCHIVE_SHARED_EXTDATA);
                            prev = SHRD_MENU;
                            state = FLDR_MENU;
                        }
                    }
                    break;
            }
        }
        else if(down & KEY_B)
            state = MAIN_MENU;

        gfx::frameBegin();
        gfx::frameStartTop();
        drawTopBar("追加数据");
        sharedMenu.draw(40, 60, 0xFFFFFFFF, 320, false);
        gfx::frameStartBot();
        gfx::drawTextWrap(sharedDesc[sharedMenu.getSelected()], 0, 0, 240, 0xFFFFFFFF);
        gfx::frameEnd();
    }

    void runApp(const uint32_t& down, const uint32_t& held)
    {
        switch(state)
        {
            case MAIN_MENU:
                stateMainMenu(down, held);
                break;

            case TITLE_MENU:
                stateTitleMenu(down, held);
                break;

            case BACK_MENU:
                stateBackupMenu(down, held);
                break;

            case SYS_MENU:
                stateNandMenu(down, held);
                break;

            case SYS_BAKMENU:
                stateNandBack(down, held);
                break;

            case FLDR_MENU:
                stateFolderMenu(down, held);
                break;

            case ADV_MENU:
                stateAdvMode(down, held);
                break;

            case SHRD_MENU:
                stateSharedSelect(down, held);
                break;
        }
    }

    void showMessage(const char *fmt, ...)
    {
        char tmp[512];
        va_list args;
        va_start(args, fmt);
        vsprintf(tmp, fmt, args);

        ui:: button ok("OK \ue000", 96, 192, 128, 32);
        while(1)
        {
            hidScanInput();

            uint32_t down = hidKeysDown();
            touchPosition p;
            hidTouchRead(&p);

            ok.update(p);

            if(down & KEY_A || ok.getEvent() == BUTTON_RELEASED)
                break;

            gfx::frameBegin();
            gfx::frameStartBot();
            C2D_DrawRectSolid(8, 8, 0.5f, 304, 224, 0xFFE7E7E7);
            ok.draw();
            gfx::drawTextWrap(tmp, 16, 16, 300, 0xFF000000);
            gfx::frameEnd();
        }
    }

    progressBar::progressBar(const uint32_t& _max)
    {
        max = (float)_max;
    }

    void progressBar::update(const uint32_t& _prog)
    {
        prog = (float)_prog;

        float percent = (float)(prog / max) * 100;
        width  = (float)(percent * 288) / 100;
    }

    void progressBar::draw(const std::string& text)
    {
        C2D_DrawRectSolid(8, 8, 0.5f, 304, 224, 0xFFE7E7E7);
        C2D_DrawRectSolid(16, 200, 0.5f, 288, 16, 0xFF000000);
        C2D_DrawRectSolid(16, 200, 0.5f, width, 16, 0xFF00FF00);
        gfx::drawTextWrap(text, 16, 16, 224, 0xFF000000);
    }

    bool confirm(const std::string& mess)
    {
        button yes("Yes \ue000", 16, 192, 128, 32);
        button no("No \ue001", 176, 192, 128, 32);

        while(true)
        {
            hidScanInput();

            uint32_t down = hidKeysDown();
            touchPosition p;
            hidTouchRead(&p);

            //Oops
            yes.update(p);
            no.update(p);

            if(down & KEY_A || yes.getEvent() == BUTTON_RELEASED)
                return true;
            else if(down & KEY_B || no.getEvent() == BUTTON_RELEASED)
                return false;

            gfx::frameBegin();
            gfx::frameStartBot();
            C2D_DrawRectSolid(8, 8, 0.5f, 304, 224, 0xFFF4F4F4);
            gfx::drawTextWrap(mess, 16, 16, 300, 0xFF000000);
            yes.draw();
            no.draw();
            gfx::frameEnd();
        }
        return false;
    }
}
