import requests
import json

url_template = "https://thanhnien.vn/api/get-data-tuyen-sinh.htm?keywords={keyword}&pageindex=1&size=10&type=3"
output_file = "results.json"

start = 1000001
end = 19019653

with open(output_file, "w", encoding="utf-8") as f:
    f.write("[\n")

    first_item = True

    for i in range(start, end):
        keyword = f"{i:08d}"

        url = url_template.format(keyword=keyword)
        try:
            response = requests.get(url)
            response.raise_for_status()
            data = response.json()

            if not first_item:
                f.write(",\n")
            else:
                first_item = False

            json_str = json.dumps(data, ensure_ascii=False, indent=2)

            indented_str = "  " + json_str.replace("\n", "\n  ")
            f.write(indented_str)

            print(f"Fetched {keyword}")
        except requests.RequestException as e:
            print(f"Request failed for keyword {keyword}: {e}")
        except json.JSONDecodeError as e:
            print(f"JSON decode failed for keyword {keyword}: {e}")

    f.write("\n]\n")
