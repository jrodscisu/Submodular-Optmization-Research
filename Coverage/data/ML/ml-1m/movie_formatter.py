def reformat_movie_data(input_file, output_file):
    try:
        with open(input_file, 'r', encoding='latin-1') as infile, open(output_file, 'w', encoding='utf-8') as outfile:
            for line in infile:
                # Remove whitespace and split by the double-colon delimiter
                parts = line.strip().split('::')
                
                # Check if the line has the expected 3 parts
                if len(parts) >= 3:
                    movie_id = parts[0]
                    genres = parts[2].replace('|', ' ')

                    
                    # Write the new format: "MovieID UserID"
                    outfile.write(f"{movie_id} {genres}\n")
        
        print(f"Successfully processed! Check '{output_file}' for the results.")
        
    except FileNotFoundError:
        print("Error: The input file was not found.")
    except Exception as e:
        print(f"An error occurred: {e}")

# Usage
reformat_movie_data('movies.dat', 'bygenre/formatted_movies.txt')