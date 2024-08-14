#include "pch.h"
#include "Menu.h"

namespace Menuc
{
	void RenderImGuiMenu(bool& bOpen)
	{
		ImGui::Begin("Hello, World!", &bOpen);
		ImGui::End();
	}
}
