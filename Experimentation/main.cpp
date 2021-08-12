#include "stdafx.h"

#include "Application.h"

int main(){
	Application app;

	try {
		app.init();

		app.run();
	}
	catch (HrException e) {
		
	}
}