#pragma once

class EditorBase {
public:
	void CreateWindowMenu();
	void CreateEditorViewport();
	void ConstructEditorLayout();
	void ConstructContentBrowser();

public:
	bool showViewport = true;
	bool showContentBrowser = true;
};