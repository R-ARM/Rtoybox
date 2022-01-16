void *initSettingsList(int id) 
{ 
	log_debug("Initializing sets list\n"); 
	FILE *settings = fopen(HOME"/settings", "r");
	if (!settings)
	{
		log_err("Error reading settings!\n");
	}	
	struct Item *current;
	int ret;
	char tmp[64];
	char tmp2[64];
	char tmp3[64];
	char tmp4[64];

	setsHead = (struct Item*)malloc(sizeof(struct Item)); 
	setsHead->type = ITEM_TYPE_PARENT; 
	strcpy(setsHead->name, "Settings");
	parents[id] = setsHead;
	
	get_text_and_rect(renderer, "Settings", font, &setsHead->texture, &setsHead->rect, 255, 255, 255); 
	
	current = setsHead; 
	
	while (1) 
	{

		current->next = (struct Item*)malloc(sizeof(struct Item)); 
		current->next->type = ITEM_TYPE_PARENT; 
		current->next->prev = current;
		do
		{
#if 1
			ret = fscanf(settings, "%s %s %d %d", current->next->name, current->next->extraPath, &current->next->subtype, &current->next->type);
#else
			ret = fscanf(settings, "%s %s %s", current->next->mainPath, current->next->name, current->next->extraPath);
#endif
		} while (strncmp(current->next->mainPath, "#", 1) == 0);
#if 1
		if (current->next->type == ITEM_TYPE_LIST)
		{
			current->next->listHead = (struct listMember*)malloc(sizeof(struct listMember));

			struct listMember *firstElem;
			struct listMember *lastElem;
			struct listMember *currentList;
			//struct listMember *currentList = (struct listMember*)malloc(sizeof(struct listMember));
			currentList = current->next->listHead;
			firstElem = currentList;
			while (1)
			{
				fscanf(settings, "%s", tmp);
				if (strncmp(tmp, "}", 1) == 0)
				{
					//free(currentList->next);
					break;
				}


				if (strncmp(tmp, "col", 3) == 0)
				{
					fscanf(settings, "%s %d %d %d", currentList->name, &currentList->val1, &currentList->val2, &currentList->val3);
				}
				currentList->next = (struct listMember*)malloc(sizeof(struct listMember));
				currentList = currentList->next;
			}
			lastElem = currentList;
			lastElem->next = firstElem;
		}

#endif

		if (feof(settings) != 0)
		{ 
			free(current->next); 
			current->next = NULL; 
			break; 
		} 
		get_text_and_rect(renderer, current->next->name, font, &current->next->texture, &current->next->rect, 255, 255, 255); 
		current->next->next = NULL; 
		current = current->next; 
	} 
	
	log_debug("Done intializing sets list\n"); 
	fclose(settings); 
}
