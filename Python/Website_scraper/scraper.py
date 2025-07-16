import requests
import json

url_template = "https://thanhnien.vn/api/get-data-tuyen-sinh.htm?keywords={keyword}&pageindex=1&size=10&type=2"
output_file = "results.json"

start = 2500
end = 7357

with open(output_file, "w", encoding="utf-8") as f:
    f.write("[\n")

    for i in range(start, end):
        keyword = str(i)
        url = url_template.format(keyword=keyword)
        try:
            response = requests.get(url)
            response.raise_for_status()
            data = response.json()

            # Dump the JSON as a string with indent=2
            json_str = json.dumps(data, ensure_ascii=False, indent=2)

            # Add two spaces of indentation to each line
            indented_str = "  " + json_str.replace("\n", "\n  ")
            f.write(indented_str)

            if i < end - 1:
                f.write(",\n")
            else:
                f.write("\n")

            print(f"Fetched {keyword}")
        except requests.RequestException as e:
            print(f"Request failed for keyword {keyword}: {e}")
        except json.JSONDecodeError as e:
            print(f"JSON decode failed for keyword {keyword}: {e}")

    f.write("]\n")
