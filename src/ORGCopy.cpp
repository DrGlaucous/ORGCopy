// To copy tracks from one ORG program to another
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
//#include <stdlib.h>//originally intended to improve the terminal experience, but dropped in favor of developing a full GUI version of this program instead
#include <cstring>
#include <sstream>
#include <vector>

#include "ORGCopy.h"
#include "File.h"

#define PRGMVERSION "1.0.3"
#define READ_LE16(p) ((p[1] << 8) | p[0]); p += 2
#define READ_LE32(p) ((p[3] << 24) | (p[2] << 16) | (p[1] << 8) | p[0]); p += 4


TRACKINFO tracks[MAXTRACK];
ORGFILES orgs[2];

//used to determine the new beat structure of the song
int gcd(int a, int b) {
	//gcd: greatest common difference
	if (b == 0)
		return a;
	return gcd(b, a % b);
}
int LeastCommonMultiple(int num1, int num2)
{
	return (num1 * num2) / gcd(num1, num2);

}

//change the timing of a song
void StretchSong(unsigned char *memfile, char bpmStretch, char dotStretch)
{

	//takes the signature and adjusts it accordingly
	memfile[8] = memfile[8] * bpmStretch;
	memfile[9] = memfile[9] * dotStretch;

	//adjusts tempo (bytes 6 and 7)
	short newTempo = (memfile[7] << 8) | memfile[6];//read data into our short
	newTempo = newTempo / (bpmStretch * dotStretch);//set the tempo relative to the stretch value
	for (unsigned int i = 0; i < 2; ++i)//push the updated values back to the data
		memfile[i + 6] = newTempo >> (8 * i);


	//we need to change the X value of all the notes and the leingth of all the notes
	//we need to use note_num to determine the note ammount in each track



	memfile += 18;//end of header data
	//iterate through all tracks and get the number of notes in each one
	for (int i = 0; i < MAXTRACK; i++)
	{
		memfile +=  4;//jump to note_num value

		tracks[i].note_num = READ_LE16(memfile);
	}

	//iterate through all tracks to append new note values
	for (int i = 0; i < MAXTRACK; i++)
	{

		//this makes the new x values
		for (int j = 0; j < tracks[i].note_num; ++j)//for each note
		{

			int writeOut = READ_LE32(memfile);//get x value and multiply it by the total stretch value
			writeOut *= (bpmStretch * dotStretch);

			memfile -= 4;//undo the previous advance

			for (unsigned int i = 0; i < 4; ++i)//write to the memfile
				memfile[i] = writeOut >> (8 * i);

			memfile += 4;//advance to next value


		}

		memfile += tracks[i].note_num;//jump over Y value and go to length value

		//this makes new length values
		for (int j = 0; j < tracks[i].note_num; ++j)//for each note
		{
			char writeOut = memfile[0];//get the value of the leingth
			writeOut *= (bpmStretch * dotStretch);

			memfile[0] = writeOut;
			++memfile;//advance to next number


		}

		memfile += (tracks[i].note_num * 2);//skip to end of data
	}

}

//checks (and removes) any "s or 's around the file path
void CheckForQuote(std::string *inpath)
{
	std::string path = *inpath;

	//check to see if the path is enclosed in " or '
	if (path.size() > 0)
	{
		if (
			((*path.begin() == '\"') &&
				(*(path.end() - 1) == '\"')) ||
			((*path.begin() == '\'') &&
				(*(path.end() - 1) == '\''))
			)
		{
			path = path.substr(1, path.size() - 2);//remove the enclosing ""s from the input
		}
	}

	*inpath = path;


}

char pass[7] = "Org-01";
char pass2[7] = "Org-02";	// Pipi
char pass3[7] = "Org-03";

//tells if the file is an ORG or not
bool VerifyFile(const char* path)
{



	FILE* file = fopen(path, "rb");
	
	if(file == NULL)
	{
		return false;
	}

	char header[6];
	
	fread(header, 1, 6, file);//get header

	if (memcmp(header, pass, 6) != 0 &&
		memcmp(header, pass2, 6) != 0 &&
		memcmp(header, pass3, 6) != 0
		)//see if the header matches either orgv1, orgv2, or orgv3
	{
		return false;
	}

	fclose(file);

	return true;
}

//tells if the selected track already has notes in it
bool PopulatedTrack(const char* path, int trackNum)
{
	FILE* file = fopen(path, "rb");

	if (file == NULL)
	{
		return false;
	}


	fseek(file, 18 + 4 + (trackNum * 6), SEEK_SET);//go to note number
	
	if (File_ReadLE16(file))//read note number, and return true if there are any notes in the track
	{
		return true;
	}

	return false;

}

bool SortFunction(NOTEDATA one, NOTEDATA two)
{
	return (one.x < two.x);

}

bool CopyOrgData(std::string Path1, std::string Path2, unsigned int TrackCopy, unsigned int TrackPaste, bool MASH, int PrioFile)
{

	memset(orgs, 0, sizeof(orgs));//ensure default values are 0
	memset(tracks, 0, sizeof(tracks));

	size_t file_size_1;//copied file
	size_t file_size_2;//copy to file
	unsigned char* memfile[4];

	memfile[0] = LoadFileToMemory(Path1.c_str(), &file_size_1);//track 1 (the one we are reading)
	memfile[1] = LoadFileToMemory(Path2.c_str(), &file_size_2);//track 2 (the one we are also reading, but with the intent of swapping info inside with stuff from file 1)
	memfile[2] = memfile[0] + 18;//used to restore the position of the pointers to the start of the file
	memfile[3] = memfile[1] + 18;

	//Path2 = Path2 + "2.org";//used for debugging
	FILE* file = fopen(Path2.c_str(), "wb");//"wb");//open new org file for writing

	if (memfile[0] == NULL || memfile[1] == NULL)
	{
		return false;
	}


	int BLCM = LeastCommonMultiple(memfile[0][8], memfile[1][8]);//beats per measure
	int DLCM = LeastCommonMultiple(memfile[0][9], memfile[1][9]);//notes per beat
	StretchSong(memfile[0], BLCM/memfile[0][8], DLCM/memfile[0][9]);//find the multiplier by dividing the LCM by the value in question
	StretchSong(memfile[1], BLCM/memfile[1][8], DLCM/memfile[1][9]);


	//std::cout << (int)memfile[1][8];//beats per measure
	//std::cout << (int)memfile[1][9];//notes in each beat


	fwrite(memfile[1], 18, 1, file);//write the header

	memfile[1] += 18;//end of header data
	memfile[0] += 18;
	
	//copy the note count up to the copied track (because we don't need anything after this)
	for (unsigned int i = 0; i < TrackCopy; ++i)
	{
		orgs[0].tracks[i].note_num = (memfile[0][5] << 8) | memfile[0][4];
		memfile[0] += 6;
	}
	//iterate to the position of the copied track
	orgs[0].tracks[TrackCopy].note_num = (memfile[0][5] << 8) | memfile[0][4];//read the note ammount of this track, but do not advance, so the data can be written in the function below





	//if we are combining tracks, we have to do things a little differently
	if (MASH)
	{
		std::vector<NOTEDATA> TrackNotes[3];//the extra track is the mashed product of the first 2... we could probably simplify this and put ALL notes into the combined vector to begin with...
		NOTEDATA BufferNote;
		//int NoteCount[2];
		unsigned int TracksCP[2] = { TrackCopy, TrackPaste };//so we can iterate through them
		memset(&BufferNote, 0, sizeof(BufferNote));


		memfile[2] = memfile[0];//stash memfile2 at the copied track's header (so we can write it back later)
		memfile[3] = memfile[1];//stash memfile3 at the start of the header

		//WRITE all headers until the paste destination, then advance the pointer to the end of the header
		for (unsigned int i = 0; i < MAXTRACK; ++i)
		{
			if (i < TrackPaste)
			{
				fwrite(memfile[1], 6, 1, file);//write the headers of everything up to the target track
				memfile[3] += 6;//advance post-header pointer to next track (to finish header writeback after we perform MASH calculations)
			}


			orgs[1].tracks[i].note_num = (memfile[1][5] << 8) | memfile[1][4];//read the note ammount of this track
			memfile[1] += 6;//advance to next track

		}
		memfile[0] += ((size_t)(MAXTRACK - TrackCopy) * 6);//iterate to the end of the header




		//we moved the read memfile to the start of the conflicting track, so we will move the writing one there, too
		//will write out everything UP TO the conflicting track (and leave the pointer there so we can merge both data at that track)
		for (unsigned int i = 0; i < TrackPaste; ++i)
		{
			//fwrite(memfile[1], (8 * orgs[1].tracks[i].note_num), 1, file);//write the notes for this track (the header isnt done yet, we can't do this)

			memfile[1] += (8 * orgs[1].tracks[i].note_num);//move pointer forward

		}
		//send the memfile to the start of the track to be copied (recall that memfile0 is the one we read ONLY)
		for (unsigned int i = 0; i < TrackCopy; ++i)
		{
			memfile[0] += (orgs[0].tracks[i].note_num * 8);
		}




		//copy note data into our vectors (moves memfile pointers to the end of the data chunk)
		//for both tracks
		//the track copied last has the highest priority
		for (int i = 0; i < 2; ++i)
		{

			//for every note in the vector we are copying
			//x and trackID
			for (int j = 0; j < orgs[i].tracks[TracksCP[i]].note_num; ++j)
			{
				//copy X to BufferNote
				BufferNote.x =
					memfile[i][3] << 24 |
					memfile[i][2] << 16 |
					memfile[i][1] << 8 |
					memfile[i][0];
				memfile[i] += 4;

				BufferNote.trackPrio = i;

				TrackNotes[i].push_back(BufferNote);//append vector
			}

			//y
			for (int j = 0; j < orgs[i].tracks[TracksCP[i]].note_num; ++j)
			{
				(TrackNotes[i].begin() + j)->y = memfile[i][0];
				memfile[i] += 1;
			}
			//length
			for (int j = 0; j < orgs[i].tracks[TracksCP[i]].note_num; ++j)
			{
				(TrackNotes[i].begin() + j)->length = memfile[i][0];
				memfile[i] += 1;
			}
			//volume
			for (int j = 0; j < orgs[i].tracks[TracksCP[i]].note_num; ++j)
			{
				(TrackNotes[i].begin() + j)->volume = memfile[i][0];
				memfile[i] += 1;
			}
			//pan
			for (int j = 0; j < orgs[i].tracks[TracksCP[i]].note_num; ++j)
			{
				(TrackNotes[i].begin() + j)->pan = memfile[i][0];
				memfile[i] += 1;
			}


		}



		//Merge vectors

		//if note x + length is greater than the next note's x value, 
		//if next note's length is greater than note's x + length, 

		//trackNotes0 will have priority

		//push everything into vector 3
		for (int p = 0; p < 2; ++p)
		{
			for (int i = 0; i < TrackNotes[p].size(); ++i)
			{
				TrackNotes[2].push_back(*(TrackNotes[p].begin() + i));
			}
		}

		std::sort(TrackNotes[2].begin(), TrackNotes[2].end(), SortFunction);//arranges them in order of X value

		//deletes entries based on priority and conflicting X and length values
		for (int i = 1; i < TrackNotes[2].size(); ++i)
		{

			//if i is out of range for either of our vectors, we simply add the rest of the other one. If both are out of range, we stop the copy (because it is finished)
			/*
			if (i >= TrackNotes[0].size())
			{
				if (i >= TrackNotes[1].size())
				{
					break;
				}
				TrackNotes[2].push_back(*(TrackNotes[1].begin() + i));
				continue;

			}
			else if (i >= TrackNotes[1].size())
			{
				TrackNotes[2].push_back(*(TrackNotes[0].begin() + i));
				continue;
			}
			*/

			//if current note start value is before the last note has ended
			if ((TrackNotes[2].begin() + i)->x < ((TrackNotes[2].begin() + i - 1)->x + (TrackNotes[2].begin() + i - 1)->length))
			{
				//if the priority number of the current note is less than that of the previous note
				if ((TrackNotes[2].begin() + i)->trackPrio < (TrackNotes[2].begin() + i - 1)->trackPrio)
				{
					//changes what note is erased based on what the user entered (1 gives top prio to the smaller, 2 gives top prio to the bigger)
					//this could probably be optimized. oh, well.
					if (PrioFile == 2)
					{
						TrackNotes[2].erase((TrackNotes[2].begin() + i));//erase the current note
					}
					else
					{
						TrackNotes[2].erase((TrackNotes[2].begin() + i - 1));//erase the other note
					}

				}
				else//the current priority number is bigger
				{
					if (PrioFile == 2)
					{
						TrackNotes[2].erase((TrackNotes[2].begin() + i - 1));//erase the other note
					}
					else
					{
						TrackNotes[2].erase((TrackNotes[2].begin() + i));//erase the current note
					}



				}

				i = 1;//start over (not starting over may give us skip-over errors)
			}


			//(TrackNotes[2].begin() + i)

			//TrackNotes[2].push_back(*(TrackNotes[0].begin() + i));


		}





		//finish writing out

		//finish the header
		for (unsigned int i = TrackPaste; i < MAXTRACK; ++i)
		{
			if (i > TrackPaste)
			{
				fwrite(memfile[3], 6, 1, file);//write the headers of everything past the target track
			}
			else
			{
				fwrite(memfile[2], 4, 1, file);//writes all except note count
				File_WriteLE16(TrackNotes[2].size(), file);//writes the new note count
			}

			memfile[3] += 6;//advance to next track

		}


		//write all the notes up to the special track
		for (unsigned int i = 0; i < TrackPaste; ++i)
		{
			fwrite(memfile[3], (8 * orgs[1].tracks[i].note_num), 1, file);//write the notes for this track

			memfile[3] += (8 * orgs[1].tracks[i].note_num);//move pointer forward

		}



		for (int i = 0; i < TrackNotes[2].size(); ++i)
			File_WriteLE32((TrackNotes[2].begin() + i)->x, file);
		for (int i = 0; i < TrackNotes[2].size(); ++i)
			File_WriteLE8((TrackNotes[2].begin() + i)->y, file);
		for (int i = 0; i < TrackNotes[2].size(); ++i)
			File_WriteLE8((TrackNotes[2].begin() + i)->length, file);
		for (int i = 0; i < TrackNotes[2].size(); ++i)
			File_WriteLE8((TrackNotes[2].begin() + i)->volume, file);
		for (int i = 0; i < TrackNotes[2].size(); ++i)
			File_WriteLE8((TrackNotes[2].begin() + i)->pan, file);



		//memfile[1] += (orgs[1].tracks[TrackPaste].note_num * 8);//move to the end of the data chunk (recall that the memory file's data didn't change with the operations above) (also recall that in reading from this pointer, we already moved it forward)

		for (int i = TrackPaste + 1; i < MAXTRACK; ++i)//iterate through the rest of the unwritten notes
		{

			fwrite(memfile[1], (8 * orgs[1].tracks[i].note_num), 1, file);//write the notes for this track			
			memfile[1] += (8 * orgs[1].tracks[i].note_num);//move pointer forward
		}




	}
	else//traditional copy method
	{

		//copy header data for the second memfile only (because the operation also writes out, and we need to already know the trackdata for the copied file)
		for (int i = 0; i < MAXTRACK; ++i)
		{
			if (i == TrackPaste)
			{
				fwrite(memfile[0], 6, 1, file);//write the contents of the first file instead
			}
			else
			{
				fwrite(memfile[1], 6, 1, file);//write the track info from the 2nd file
			}


			orgs[1].tracks[i].note_num = (memfile[1][5] << 8) | memfile[1][4];//read the note ammount of this track
			memfile[1] += 6;//advance to next track

		}
		memfile[0] += ((size_t)(MAXTRACK - TrackCopy) * 6);//iterate to the end of the header


		//send the memfile to the start of the track to be copied (recall that memfile0 is the one we read ONLY)
		for (unsigned int i = 0; i < TrackCopy; ++i)
		{
			memfile[0] += (orgs[0].tracks[i].note_num * 8);
		}



		//copy note data
		for (int i = 0; i < MAXTRACK; ++i)
		{
			if (i == TrackPaste)
			{

				fwrite(memfile[0], (8 * orgs[0].tracks[TrackCopy].note_num), 1, file);//write the notes for this track
			}
			else
			{
				fwrite(memfile[1], (8 * orgs[1].tracks[i].note_num), 1, file);//write the notes for this track
			}

			memfile[1] += (8 * orgs[1].tracks[i].note_num);//move pointer forward

		}

		//load file 2 to memory,
		//stream file 1, find track data
		//rebuild file 2 from memory
		//unsigned char* cutmemfile = DeleteByteSection(memfile, file_size_2, 0, 0);



		//WriteFileFromMemory(Path2.c_str(), memfile[1], file_size_2, "wb");
	}

	fclose(file);


	return true;

}

//converts the QWERTY drum tracks into an int usable by the other functions
int ParseLetterInput(const char* input)//this function will just look at the first letter in the pointer (not a problem if you only give it one letter)
{
	switch (tolower(*input))
	{
		break;
	case 'q':
		return 9;
		break;
	case 'w':
		return 10;
		break;
	case 'e':
		return 11;
		break;
	case 'r':
		return 12;
		break;
	case 't':
		return 13;
		break;
	case 'y':
		return 14;
		break;
	case 'u':
		return 15;
		break;
	case 'i':
		return 16;
		break;
	default:
		return 0;

	}

}

int main(void)
{

	std::string InputText;

	std::string Path1;
	std::string Path2;
	int TrackCopy{};//copy from this track (from file 1)
	int TrackPaste{};//copy to this track (from file 2)
	int PrioFile{};//which file has priority in a TrackMASH
	bool TrackMASH = false;

	std::cout << "ORGCopy by Dr_Glaucous (2022) version: " << PRGMVERSION << std::endl;


	//for testing the MASH function
	//Path1 = std::string("C:\\Users\\User\\Desktop\\CaveStory RomHack\\ORGTools\\ORGCopySource\\TestSong.org");
	//Path2 = std::string("C:\\Users\\User\\Desktop\\CaveStory RomHack\\ORGTools\\ORGCopySource\\TestSong.org1.org");
	//CopyOrgData(Path1, Path2, 0, 1, true, 1);




	char confirm = 0;
	bool canExit = false;
	while (canExit == false)
	{
		TrackMASH = false;

		//get criteria for the first ORG (directory and track)
		bool ValidFirstOrg = false;
		while (ValidFirstOrg == false)
		{
			std::cout << "Please enter the ORG to be copied from." << std::endl;
			getline(std::cin, InputText);

			CheckForQuote(&InputText);

			if (VerifyFile(InputText.c_str()))//try to load the ORG (also verifies its org-iness)
			{
				std::cout << InputText << " is a an ORG file. " << std::endl;
				Path1 = InputText;//cache the path for the first file

				bool ValidTrackCopy = false;
				while (ValidTrackCopy == false)//get the track to copy from
				{
					std::cout << "Please enter the track you wish to copy." << std::endl;
					getline(std::cin, InputText);

					TrackCopy = strtol(InputText.c_str(), NULL, 10);//copy value to the cache

					if (
						(TrackCopy <= 16 && TrackCopy >= 1) ||
						(InputText.length() < 2 && (TrackCopy = ParseLetterInput(InputText.c_str())))
						)//verify that the track selected is within the range of valid tracks
					{

						std::cout << "You Selected track #" << TrackCopy << std::endl;
						ValidTrackCopy = true;

					}
					else
					{
						std::cout << "Invalid track: " << TrackCopy << "\nEnter a number between 1 and 16 or a valid drum track" << std::endl;
					}


				}


				ValidFirstOrg = true;

			}
			else//TODO: make this a warning, but still let users try to copy to and from it (to cover my butt against any more file header variations, may save this for the GUI revision)
			{
				std::cout << InputText << " is NOT recognized as an ORG file. " << std::endl;

			}

		}

		//get criteria for the second ORG (directory and track)
		bool ValidSecondOrg = false;
		while (ValidSecondOrg == false)
		{
			std::cout << "Please enter the ORG to be copied to." << std::endl;
			getline(std::cin, InputText);

			CheckForQuote(&InputText);

			if (VerifyFile(InputText.c_str()))//try to load the ORG (also verifies its org-iness)
			{
				if (InputText == Path1)//if the ORG is the same as the original
				{
					std::cout << "Error: Your destination is the same as your source." << std::endl;
				}
				else
				{

					std::cout << InputText << " is a an ORG file. " << std::endl;
					Path2 = InputText;//cache the path for the second file


					bool ValidTrackCopy = false;
					while (ValidTrackCopy == false)//get the track to copy from
					{
						std::cout << "Please enter the track you wish to overwrite." << std::endl;
						getline(std::cin, InputText);

						TrackPaste = strtol(InputText.c_str(), NULL, 10);//copy value to the cache



						if (
							(TrackPaste <= 16 && TrackPaste >= 1) ||
							(InputText.length() < 2 && (TrackPaste = ParseLetterInput(InputText.c_str())))
							)//verify that the track selected is within the range of valid tracks
						{

							std::cout << "You Selected track #" << TrackPaste << std::endl;
							

							if (PopulatedTrack(Path2.c_str(), TrackPaste - 1))
							{
								std::cout << "Warning: The selected track is already populated!"
									<< "\nDo you want to overwrite it? (y/n)" << std::endl;

								std::cin >> confirm;
								std::cin.ignore();

								if (tolower(confirm) == 'y')//the user wishes to continue
								{
									confirm = 0;
									ValidTrackCopy = true;
								}

								std::cout << "Do you want to attempt a TrackMASH? (y/n)\n"
									<< "TrackMASHing will attempt to place notes only in spots where there isn't notes already." << std::endl;

								std::cin >> confirm;
								std::cin.ignore();

								if (tolower(confirm) == 'y')//the user wishes to continue
								{

									while (1)
									{

										confirm = 0;

										std::cout << "Please select the priority file. (1, for the \"Copy From File\" /2, for the \"Pasted To File\")\n"
											<< "The notes in this file will have the right-of-way when notes are in conflict." << std::endl;

										getline(std::cin, InputText);

										PrioFile = strtol(InputText.c_str(), NULL, 10);//copy value to the cache

										if (
											PrioFile != 1 &&
											PrioFile != 2
											)//verify that the track selected is within the range of valid tracks
										{
											std::cout << "Invalid entry: " << PrioFile << "\nEnter either 1 or 2" << std::endl;
										}
										else
										{
											ValidTrackCopy = true;
											TrackMASH = true;
											break;
										}


									}


								}

							}
							else
							{
								ValidTrackCopy = true;
							}

						}
						else
						{
							std::cout << "Invalid track: " << TrackPaste << "\nEnter a number between 1 and 16 or a valid drum track" << std::endl;
						}




					}


					ValidSecondOrg = true;
				}

			}
			else
			{
				std::cout << InputText << " is NOT an ORG file. " << std::endl;

			}

		}

		std::cout << "\nYour Criteria:\n"
			<< "Copying ORG: \n"
			<< Path1.c_str() << '\n'
			<< "Track # " << TrackCopy << '\n'
			<< "\nTo: \n"
			<< Path2.c_str() << '\n'
			<< "Track # " << TrackPaste << '\n'
			<< "Combination type: ";
			
			if (TrackMASH)
			{
				std::cout << "with TrackMASH Priority: File #" << PrioFile << '\n';
			}
			else
			{
				std::cout << "Standard Overwrite\n";
			}
			
			std::cout <<"\nIs this Correct? (input \'y\' to continue)" << std::endl;

		std::cin >> confirm;
		std::cin.ignore();

		if (tolower(confirm) == 'y')//the user wishes to continue
		{
			if (CopyOrgData(Path1, Path2, TrackCopy - 1, TrackPaste - 1, TrackMASH, PrioFile))//since "track 1" is acutally track 0 in the file, we must shift these values
			{
				confirm = 0;//reset choice
				std::cout << "\nDone." << std::endl;
				//canExit = true;
			}
			else
			{
				std::cout << "\n Error: Copy failed. Were the files moved or altered?" << std::endl;
			}
		
		
		
			std::cout << "\nDo you want to copy another Track? (y/n)\n";
			std::cin >> confirm;
			std::cin.ignore();
			if (tolower(confirm) == 'y')//the user wishes to continue
			{
				confirm = 0;//reset confirm

			}
			else
			{
				canExit = true;
			}
			
		
		}


	}
	


	std::cout << "\nProcess Finished.\nFeel free to close the window or enter any value to exit." << std::endl;

	char isFinished;//this is the best and most universal way I can find to halt the terminal for user input
	std::cin >> isFinished;
	std::cin.ignore();

	return 0;
}
