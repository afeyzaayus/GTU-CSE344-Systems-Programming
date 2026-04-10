#include "get_next_line.h"
#include <unistd.h>
#include <stdlib.h>

static char	*read_file(int fd, char *data) {
	long	nbytes;
	char	*buffer;

	buffer = (char *)malloc((BUFFER_SIZE + 1) * sizeof(char));
	if (buffer == NULL)
		return (ft_free(data));
	while (1) {
		nbytes = read(fd, buffer, BUFFER_SIZE);
		if (nbytes < 0)
			return (free(data), data = NULL, NULL);
		if (nbytes == 0)
			break ;
		buffer[nbytes] = '\0';
		data = ft_join(data, buffer);
		if (!data) {
			free(buffer);
			return (ft_free(data));
		}
		if (ft_strchr(data, '\n'))
			break ;
	}
	free(buffer);
	return (data);
}

static char	*ft_sep_line(char *data) {
	long	i;
	char	*line;

	i = -1;
	if (!data)
		return (NULL);
	while (data[++i] != '\n' && data[i])
		;
	if (data[i] == '\n')
		i++;
	line = (char *)malloc((i + 1) * sizeof(char));
	if (!line)
		return (NULL);
	i = -1;
	while (data[++i] != '\n' && data[i])
		line[i] = data[i];
	if (data[i] == '\n')
		line[i++] = '\n';
	line[i] = '\0';
	return (line);
}

static char	*ft_update(char *data) {
	char	*new_data;
	long	i;
	size_t	j;

	i = -1;
	while (data[++i] != '\n' && data[i])
		;
	if (data[i] == '\n')
		i++;
	if (!data[i])
		return (ft_free(data));
	new_data = (char *)malloc(ft_strchr(&data[i], '\0') - &data[i] + 1);
	if (!new_data)
		return (ft_free(data));
	j = 0;
	while (data[i])
		new_data[j++] = data[i++];
	new_data[j] = '\0';
	return (ft_free(data), new_data);
}

char	*get_next_line(int fd)
{
	static char	*data = NULL;
	char		*line;

	if (fd < 0 || BUFFER_SIZE <= 0)
		return (0);
	data = read_file(fd, data);
	if (!data)
		return (ft_free(data));
	line = ft_sep_line(data);
	if (!line)
		return (ft_free(data), data = NULL, NULL);
	data = ft_update(data);
	return (line);
}