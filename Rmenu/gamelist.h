void *initGameList(int id) 
{ 
	log_debug("Initializing game list\n"); 
	FILE *games = fopen(HOME"/games", "r");
	if (!games)
	{
		log_err("Error reading games!\n");
	}	
	struct Item *current;
	int ret;
	char tmp[64];
	char tmp2[64];
	char tmp3[64];
	char tmp4[64];

	gameHead = (struct Item*)malloc(sizeof(struct Item)); 
	gameHead->type = ITEM_TYPE_PARENT; 
	strcpy(gameHead->name, "Game");
	parents[id] = gameHead;
	
	get_text_and_rect(renderer, "Game", font, &gameHead->texture, &gameHead->rect, 255, 255, 255); 
	
	current = gameHead; 
	
	while (1) 
	{

		current->next = (struct Item*)malloc(sizeof(struct Item)); 
		current->next->type = ITEM_TYPE_GAME; 
		current->next->prev = current;
		do
		{
#if 0
			ret = fscanf(games, "%s %s %d %d", current->next->name, current->next->extraPath, &current->next->subtype, &current->next->type);
#else
			ret = fscanf(games, "%s %s %s", current->next->mainPath, current->next->name, current->next->extraPath);
#endif
		} while (strncmp(current->next->mainPath, "#", 1) == 0);
#if 0
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
				fscanf(games, "%s", tmp);
				if (strncmp(tmp, "}", 1) == 0)
				{
					//free(currentList->next);
					break;
				}


				if (strncmp(tmp, "col", 3) == 0)
				{
					fscanf(games, "%s %d %d %d", currentList->name, &currentList->val1, &currentList->val2, &currentList->val3);
				}
				currentList->next = (struct listMember*)malloc(sizeof(struct listMember));
				currentList = currentList->next;
			}
			lastElem = currentList;
			lastElem->next = firstElem;
		}

#endif

		if (feof(games) != 0)
		{ 
			free(current->next); 
			current->next = NULL; 
			break; 
		} 
		get_text_and_rect(renderer, current->next->name, font, &current->next->texture, &current->next->rect, 255, 255, 255); 
		current->next->next = NULL; 
		current = current->next; 
	} 
	
	log_debug("Done intializing game list\n"); 
	fclose(games); 
}
