/*
 *  MStarDict - International dictionary for Maemo.
 *  Copyright (C) 2010 Roman Moravcik
 *
 *  base on code of stardict:
 *  Copyright (C) 2003-2007 Hu Zheng <huzheng_001@163.com>
 *
 *  based on code of sdcv:
 *  Copyright (C) 2005-2006 Evgeniy <dushistov@mail.ru>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

typedef struct {
    const char *name;
    const char *disp_name;
} TtsVoice;

class MStarDict;

class Tts {
  private:
    MStarDict *oStarDict;
    bool Enabled;

    GtkWidget *enable_button;
    GtkWidget *gender_male_button;
    GtkWidget *gender_female_button;
    GtkWidget *language_button;

    static gboolean onTtsEnableButtonClicked(GtkButton *button,
					     Tts *oTts);
    static gboolean onTtsGenderButtonClicked(GtkToggleButton *button1,
					     GtkToggleButton *button2);

  public:
     Tts(MStarDict *mStarDict);
    ~Tts();

    void Enable(bool bEnable);
    bool IsEnabled();

    void SetVoice(const gchar *language, int gender);
    GtkListStore *GetVoicesList();

    void SayText(const gchar *sText);

    GtkWidget *CreateTtsConfWidget();
    void TtsConfWidgetLoadConf();
    void TtsConfWidgetSaveConf();
};
